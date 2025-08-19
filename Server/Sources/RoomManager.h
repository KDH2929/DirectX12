#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

#include "GameRoom.h"
#include "ClientSession.h"

// 헤더 의존 최소화를 원하면 전방 선언 사용 가능
namespace game { class GameMessage; }

class RoomManager {
public:
    GameRoom* GetRoom(int roomId);
    const GameRoom* GetRoom(int roomId) const;

    GameRoom& GetOrCreateRoom(int roomId);

    void BroadcastGameMessageToRoom(int roomId,
        const game::GameMessage& message,
        const ClientSession* excludedSession = nullptr);

    void RemovePlayerFromAllRooms(ClientSession* clientSession);

    std::vector<GameRoom*> GetAllRoomList();
    std::vector<const GameRoom*> GetAllRoomList() const;

private:
    mutable std::mutex managerMutex;
    std::unordered_map<int, std::unique_ptr<GameRoom>> roomsById;
};
