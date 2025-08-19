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

// IOCP ��� ��Ʈ��ũ ����
// - Accept / Recv / Send �� IOCP �񵿱�� ó��
// - ���� ���� ���۴� �� ClientSession �� ����
class IocpServer {
public:
    explicit IocpServer(MessageDispatcher& messageDispatcherRef);
    ~IocpServer();

    bool Initialize(const char* bindAddress, unsigned short bindPort, int workerThreadCount);
    void Stop();

    bool IsRunning() const { return running.load(std::memory_order_acquire); }

private:
    // �ʱ�ȭ
    bool InitializeWinsock();
    bool CreateListenSocket(const char* bindAddress, unsigned short bindPort);
    bool CreateCompletionPort();
    bool LoadAcceptExFunctions();
    bool PostInitialAccepts();

    // I/O ����
    bool SubmitAcceptOperation();
    bool SubmitRecvOperation(const std::shared_ptr<ClientSession>& session);

    // ��Ŀ
    void WorkerLoop();

    // �Ϸ� ó��
    void OnAcceptCompleted(IocpIoContext* context, DWORD transferredBytes);
    void OnRecvCompleted(IocpIoContext* context, DWORD transferredBytes);
    void OnSendCompleted(IocpIoContext* context, DWORD transferredBytes);

    // ��ƿ
    void AssociateSocketWithIocp(SOCKET socketHandle);
    void RegisterSession(const std::shared_ptr<ClientSession>& session);
    void UnregisterSession(SOCKET socketHandle);
    void CloseSession(const std::shared_ptr<ClientSession>& session);

private:
    // ����/IOCP
    SOCKET listenSocket{ INVALID_SOCKET };
    HANDLE completionPort{ nullptr };

    // Ȯ�� �Լ�
    LPFN_ACCEPTEX lpfnAcceptEx{ nullptr };
    LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs{ nullptr };

    // ���� ����
    std::atomic<bool> shuttingDown{ false };
    std::atomic<bool> running{ false };

    // ��Ŀ ������
    std::vector<std::thread> workerThreads;

    // ���� ������Ʈ��
    std::mutex sessionsMutex;
    std::unordered_map<SOCKET, std::shared_ptr<ClientSession>> sessions;

    // ������
    MessageDispatcher& messageDispatcher;

    // ���
    static constexpr int    PrepostedAcceptCount = 64;
    static constexpr size_t AcceptAddressSpaceBytes = 2 * (sizeof(SOCKADDR_IN6) + 16);
    static constexpr size_t DefaultRecvBufferBytes = 16 * 1024;
};
