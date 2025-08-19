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

    // 수신 경로
    void AppendToReceiveBuffer(const char* dataPointer, size_t sizeBytes);
    // 누적 버퍼에서 가능한 모든 메시지를 꺼내어 디스패치
    // NeedMoreData면 true, 치명 오류 시 false(상위에서 Close)
    bool TryExtractAndDispatchAllMessages(MessageDispatcher& dispatcher);

    // 송신 경로
    void EnqueueSend(const std::string& bytes);
    void TrySendNext();  // IOCP send 완료 콜백에서 재호출

    // GameMessage 전송 유틸
    void SendGameMessage(const game::GameMessage& message);

    // 로그인 응답 전송 (Result.status_code 기반)
    void SendLoginResponse(game::StatusCode statusCode,
        const std::string& statusMessage,
        std::uint32_t playerId,
        const std::string& sessionToken);

    // 세션 상태(필요 최소)
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

    // 누적 수신 버퍼
    std::string receiveBuffer;

    // 송신 직렬화 큐
    std::mutex sendQueueMutex;
    std::deque<std::string> sendQueue;
    std::atomic<bool> sendingInProgress{ false };
};
