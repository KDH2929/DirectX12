#include "IocpServer.h"
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

namespace {
    inline void LogSystemError(const char* where, int error) {
        std::cerr << "[IOCP] " << where << " failed. WSAGetLastError=" << error << "\n";
    }
}

IocpServer::IocpServer(MessageDispatcher& messageDispatcherRef)
    : messageDispatcher(messageDispatcherRef) {}

IocpServer::~IocpServer() {
    Stop();
}

bool IocpServer::Initialize(const char* bindAddress, unsigned short bindPort, int workerThreadCount) {
    if (running.load(std::memory_order_acquire)) return true;

    if (!InitializeWinsock()) return false;
    if (!CreateListenSocket(bindAddress, bindPort)) return false;
    if (!CreateCompletionPort()) return false;
    if (!LoadAcceptExFunctions()) return false;
    if (!PostInitialAccepts()) return false;

    shuttingDown.store(false, std::memory_order_release);
    running.store(true, std::memory_order_release);

    if (workerThreadCount <= 0) {
        SYSTEM_INFO systemInfo{};
        GetSystemInfo(&systemInfo);
        workerThreadCount = static_cast<int>(systemInfo.dwNumberOfProcessors) * 2;
    }

    workerThreads.reserve(static_cast<size_t>(workerThreadCount));
    for (int i = 0; i < workerThreadCount; ++i) {
        workerThreads.emplace_back(&IocpServer::WorkerLoop, this);
    }

    std::cout << "[IOCP] Server initialized on " << bindAddress << ":" << bindPort
        << " (threads=" << workerThreadCount << ")\n";
    return true;
}

void IocpServer::Stop() {
    if (!running.load(std::memory_order_acquire)) return;

    shuttingDown.store(true, std::memory_order_release);

    // 새로운 수락 중단
    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }

    // 모든 세션 I/O 취소 및 소켓 닫기
    {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        for (auto& entry : sessions) {
            SOCKET socketHandle = entry.first;
            const auto& session = entry.second;
            CancelIoEx(reinterpret_cast<HANDLE>(socketHandle), nullptr);
            if (session) session->Close();
        }
        sessions.clear();
    }

    // 워커 깨우기
    for (size_t i = 0; i < workerThreads.size(); ++i) {
        PostQueuedCompletionStatus(completionPort, 0, 0, nullptr);
    }
    for (auto& thread : workerThreads) {
        if (thread.joinable()) thread.join();
    }
    workerThreads.clear();

    if (completionPort) {
        CloseHandle(completionPort);
        completionPort = nullptr;
    }

    WSACleanup();
    running.store(false, std::memory_order_release);
    std::cout << "[IOCP] Server stopped.\n";
}

bool IocpServer::InitializeWinsock() {
    WSADATA wsaData{};
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        LogSystemError("WSAStartup", result);
        return false;
    }
    return true;
}

bool IocpServer::CreateListenSocket(const char* bindAddress, unsigned short bindPort) {
    listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (listenSocket == INVALID_SOCKET) {
        LogSystemError("WSASocket(listen)", WSAGetLastError());
        return false;
    }

    BOOL reuse = TRUE;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(bindAddress); // 예: "0.0.0.0"
    address.sin_port = htons(bindPort);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        LogSystemError("bind", WSAGetLastError());
        return false;
    }
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        LogSystemError("listen", WSAGetLastError());
        return false;
    }
    return true;
}

bool IocpServer::CreateCompletionPort() {
    completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!completionPort) {
        std::cerr << "[IOCP] CreateIoCompletionPort failed.\n";
        return false;
    }
    AssociateSocketWithIocp(listenSocket);
    return true;
}

void IocpServer::AssociateSocketWithIocp(SOCKET socketHandle) {
    HANDLE associationHandle = CreateIoCompletionPort(reinterpret_cast<HANDLE>(socketHandle), completionPort, 0, 0);
    if (associationHandle != completionPort) {
        std::cerr << "[IOCP] Failed to associate socket with IOCP.\n";
    }
}

bool IocpServer::LoadAcceptExFunctions() {
    DWORD bytesReturned = 0;

    GUID guidAcceptEx = WSAID_ACCEPTEX;
    if (WSAIoctl(listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidAcceptEx, sizeof(guidAcceptEx),
        &lpfnAcceptEx, sizeof(lpfnAcceptEx),
        &bytesReturned, nullptr, nullptr) == SOCKET_ERROR) {
        LogSystemError("WSAIoctl(AcceptEx)", WSAGetLastError());
        return false;
    }

    GUID guidGetAddrs = WSAID_GETACCEPTEXSOCKADDRS;
    if (WSAIoctl(listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidGetAddrs, sizeof(guidGetAddrs),
        &lpfnGetAcceptExSockaddrs, sizeof(lpfnGetAcceptExSockaddrs),
        &bytesReturned, nullptr, nullptr) == SOCKET_ERROR) {
        LogSystemError("WSAIoctl(GetAcceptExSockaddrs)", WSAGetLastError());
        return false;
    }
    return true;
}

bool IocpServer::PostInitialAccepts() {
    for (int i = 0; i < PrepostedAcceptCount; ++i) {
        if (!SubmitAcceptOperation()) return false;
    }
    return true;
}

bool IocpServer::SubmitAcceptOperation() {
    auto* context = new IocpIoContext();
    context->Reset(IoOperationType::Accept, AcceptAddressSpaceBytes);

    context->acceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (context->acceptSocket == INVALID_SOCKET) {
        LogSystemError("WSASocket(accept)", WSAGetLastError());
        delete context;
        return false;
    }

    DWORD bytesReceived = 0;
    if (!lpfnAcceptEx(
        listenSocket,
        context->acceptSocket,
        context->buffer.data(),
        0,
        static_cast<DWORD>(AcceptAddressSpaceBytes / 2),
        static_cast<DWORD>(AcceptAddressSpaceBytes / 2),
        &bytesReceived,
        &context->overlapped))
    {
        int error = WSAGetLastError();
        if (error != ERROR_IO_PENDING) {
            LogSystemError("AcceptEx", error);
            closesocket(context->acceptSocket);
            delete context;
            return false;
        }
    }
    return true;
}

void IocpServer::WorkerLoop() {
    while (!shuttingDown.load(std::memory_order_acquire)) {
        DWORD transferredBytes = 0;
        ULONG_PTR completionKey = 0;
        OVERLAPPED* overlappedPtr = nullptr;

        BOOL operationSucceeded = GetQueuedCompletionStatus(
            completionPort, &transferredBytes, &completionKey, &overlappedPtr, INFINITE);

        if (shuttingDown.load(std::memory_order_acquire)) break;

        IocpIoContext* context = reinterpret_cast<IocpIoContext*>(overlappedPtr);

        if (!operationSucceeded) {
            if (context && context->ownerSession) {
                CloseSession(context->ownerSession);
            }
            delete context;
            continue;
        }
        if (!context) continue;

        switch (context->operationType) {
        case IoOperationType::Accept:
            OnAcceptCompleted(context, transferredBytes);
            break;
        case IoOperationType::Recv:
            OnRecvCompleted(context, transferredBytes);
            break;
        case IoOperationType::Send:
            OnSendCompleted(context, transferredBytes);
            break;
        }
    }
}

void IocpServer::OnAcceptCompleted(IocpIoContext* context, DWORD /*transferredBytes*/) {
    // IOCP 등록
    AssociateSocketWithIocp(context->acceptSocket);

    // 수락 컨텍스트 상속
    setsockopt(context->acceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
        reinterpret_cast<const char*>(&listenSocket), sizeof(listenSocket));

    // 지연 최소화: Nagle 끄기
    BOOL disableNagle = TRUE;
    setsockopt(context->acceptSocket, IPPROTO_TCP, TCP_NODELAY,
        reinterpret_cast<const char*>(&disableNagle), sizeof(disableNagle));

    // 세션 생성 및 등록
    auto session = std::make_shared<ClientSession>(context->acceptSocket);
    RegisterSession(session);

    // 첫 수신 제출 + 다음 Accept 재제출
    SubmitRecvOperation(session);
    SubmitAcceptOperation();

    delete context;
}

bool IocpServer::SubmitRecvOperation(const std::shared_ptr<ClientSession>& session) {
    auto* context = new IocpIoContext();
    context->Reset(IoOperationType::Recv, DefaultRecvBufferBytes);
    context->ownerSession = session;

    DWORD flags = 0;
    DWORD bytesReceived = 0;
    int result = WSARecv(session->GetSocket(), &context->wsaBuffer, 1, &bytesReceived, &flags, &context->overlapped, nullptr);
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            LogSystemError("WSARecv", error);
            delete context;
            return false;
        }
    }
    return true;
}

void IocpServer::OnRecvCompleted(IocpIoContext* context, DWORD transferredBytes) {
    auto session = context->ownerSession;
    if (!session) { delete context; return; }

    if (transferredBytes == 0) {
        CloseSession(session);
        delete context;
        return;
    }

    session->AppendToReceiveBuffer(context->buffer.data(), static_cast<size_t>(transferredBytes));

    // 누적 버퍼에서 가능한 모든 메시지를 파싱하고 디스패치
    if (!session->TryExtractAndDispatchAllMessages(messageDispatcher)) {
        CloseSession(session); // 파싱 에러 또는 정책 위반 → 종료
        delete context;
        return;
    }

    if (!SubmitRecvOperation(session)) {
        CloseSession(session);
    }

    delete context;
}

void IocpServer::OnSendCompleted(IocpIoContext* context, DWORD /*transferredBytes*/) {
    auto session = context->ownerSession;
    delete context;
    if (!session) return;
    session->TrySendNext(); // 다음 송신 이어서 처리
}

void IocpServer::RegisterSession(const std::shared_ptr<ClientSession>& session) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    sessions.emplace(session->GetSocket(), session);
}

void IocpServer::UnregisterSession(SOCKET socketHandle) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    sessions.erase(socketHandle);
}

void IocpServer::CloseSession(const std::shared_ptr<ClientSession>& session) {
    if (!session) return;
    UnregisterSession(session->GetSocket());
    session->Close();
}
