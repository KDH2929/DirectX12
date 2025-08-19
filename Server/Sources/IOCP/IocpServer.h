#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>

#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <string>

#include "IocpIoContext.h"
#include "ClientSession.h"
#include "../Message/MessageDispatcher.h"
#include "../Message/MessageSerializer.h"

// IOCP 기반 네트워크 서버
// - Accept / Recv / Send 를 IOCP 비동기로 처리
// - 누적 수신 버퍼는 각 ClientSession 이 소유
class IocpServer {
public:
    explicit IocpServer(MessageDispatcher& messageDispatcherRef);
    ~IocpServer();

    bool Initialize(const char* bindAddress, unsigned short bindPort, int workerThreadCount);
    void Stop();

    bool IsRunning() const { return running.load(std::memory_order_acquire); }

private:
    // 초기화
    bool InitializeWinsock();
    bool CreateListenSocket(const char* bindAddress, unsigned short bindPort);
    bool CreateCompletionPort();
    bool LoadAcceptExFunctions();
    bool PostInitialAccepts();

    // I/O 제출
    bool SubmitAcceptOperation();
    bool SubmitRecvOperation(const std::shared_ptr<ClientSession>& session);

    // 워커
    void WorkerLoop();

    // 완료 처리
    void OnAcceptCompleted(IocpIoContext* context, DWORD transferredBytes);
    void OnRecvCompleted(IocpIoContext* context, DWORD transferredBytes);
    void OnSendCompleted(IocpIoContext* context, DWORD transferredBytes);

    // 유틸
    void AssociateSocketWithIocp(SOCKET socketHandle);
    void RegisterSession(const std::shared_ptr<ClientSession>& session);
    void UnregisterSession(SOCKET socketHandle);
    void CloseSession(const std::shared_ptr<ClientSession>& session);

private:
    // 소켓/IOCP
    SOCKET listenSocket{ INVALID_SOCKET };
    HANDLE completionPort{ nullptr };

    // 확장 함수
    LPFN_ACCEPTEX lpfnAcceptEx{ nullptr };
    LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs{ nullptr };

    // 실행 상태
    std::atomic<bool> shuttingDown{ false };
    std::atomic<bool> running{ false };

    // 워커 스레드
    std::vector<std::thread> workerThreads;

    // 세션 레지스트리
    std::mutex sessionsMutex;
    std::unordered_map<SOCKET, std::shared_ptr<ClientSession>> sessions;

    // 의존성
    MessageDispatcher& messageDispatcher;

    // 상수
    static constexpr int    PrepostedAcceptCount = 64;
    static constexpr size_t AcceptAddressSpaceBytes = 2 * (sizeof(SOCKADDR_IN6) + 16);
    static constexpr size_t DefaultRecvBufferBytes = 16 * 1024;
};
