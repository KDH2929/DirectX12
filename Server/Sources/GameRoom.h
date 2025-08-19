#pragma once
#include <unordered_set>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <cstdint>

class ClientSession;
struct Plane;

// 프로토버퍼 헤더를 헤더파일에서 포함하지 않기 위해 전방 선언
namespace game { class GameMessage; }

class GameRoom {
public:
    explicit GameRoom(int roomId);

    void AddPlayer(ClientSession* clientSession);
    void RemovePlayer(ClientSession* clientSession);

    // 방 안의 모든 플레이어에게 GameMessage 브로드캐스트
    void BroadcastGameMessage(const game::GameMessage& message,
        const ClientSession* excludedSession = nullptr);

    int GetRoomId() const { return roomId; }
    std::vector<ClientSession*> GetPlayerList() const;  // 명칭 단순화
    int GetPlayerCount() const;

private:
    const int roomId;
    mutable std::mutex roomMutex;
    std::unordered_set<ClientSession*> players;  // 수명은 외부(Client 매니저)가 관리
};
