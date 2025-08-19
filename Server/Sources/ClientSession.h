#pragma once

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <atomic>
#include <mutex>
#include <deque>
#include <string>
#include <memory>

#include "IOCP/IocpIoContext.h"
#include "Message/MessageDispatcher.h"
#include "Message/MessageSerializer.h"
#include "Message/game_message.pb.h"

struct Vector3f { float x{ 0 }, y{ 0 }, z{ 0 }; };

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    explicit ClientSession(SOCKET socketHandle);
    ~ClientSession();

    SOCKET GetSocket() const { return socketHandle; }
    void Close();

    // ���� ���
    void AppendToReceiveBuffer(const char* dataPointer, size_t sizeBytes);
    // ���� ���ۿ��� ������ ��� �޽����� ������ ����ġ
    // NeedMoreData�� true, ġ�� ���� �� false(�������� Close)
    bool TryExtractAndDispatchAllMessages(MessageDispatcher& dispatcher);

    // �۽� ���
    void EnqueueSend(const std::string& bytes);
    void TrySendNext();  // IOCP send �Ϸ� �ݹ鿡�� ��ȣ��

    // GameMessage ���� ��ƿ
    void SendGameMessage(const game::GameMessage& message);

    // �α��� ���� ���� (Result.status_code ���)
    void SendLoginResponse(game::StatusCode statusCode,
        const std::string& statusMessage,
        std::uint32_t playerId,
        const std::string& sessionToken);

    // ���� ����(�ʿ� �ּ�)
    int playerId{ 0 };
    int roomId{ 0 };
    std::string nickname;
    Vector3f position{};
    unsigned int healthPoints{ 100 };
    void ApplyDamage(float amount) {
        healthPoints = (amount >= healthPoints) ? 0u
            : (healthPoints - static_cast<unsigned int>(amount));
    }

private:
    bool SubmitSendOperation(const std::string& bytes);

private:
    SOCKET socketHandle{ INVALID_SOCKET };
    std::atomic<bool> closed{ false };

    // ���� ���� ����
    std::string receiveBuffer;

    // �۽� ����ȭ ť
    std::mutex sendQueueMutex;
    std::deque<std::string> sendQueue;
    std::atomic<bool> sendingInProgress{ false };
};
