#pragma once
#include <winsock2.h>
#include <vector>
#include <memory>

enum class IoOperationType { Accept, Recv, Send };

class ClientSession;

struct IocpIoContext {
    OVERLAPPED overlapped{};
    WSABUF wsaBuffer{};
    std::vector<char> buffer;
    IoOperationType operationType{ IoOperationType::Recv };
    SOCKET acceptSocket{ INVALID_SOCKET };
    std::shared_ptr<ClientSession> ownerSession;

    void Reset(IoOperationType type, size_t bufferSize) {
        ZeroMemory(&overlapped, sizeof(overlapped));
        operationType = type;
        buffer.resize(bufferSize);
        wsaBuffer.buf = buffer.data();
        wsaBuffer.len = static_cast<ULONG>(buffer.size());
        acceptSocket = INVALID_SOCKET;
        ownerSession.reset();
    }
};
