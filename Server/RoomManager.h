#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include "GameRoom.h"
#include "ClientSession.h"

class RoomManager {
public:
    GameRoom* GetRoom(int roomId);
    GameRoom& GetOrCreateRoom(int roomId);
    void BroadcastToRoom(int roomId, const game::Packet& packet, ClientSession* excludeClient = nullptr);
    void RemovePlayerFromAllRooms(ClientSession* client);
    std::vector<GameRoom*> GetAllRooms();


private:
    std::mutex mutex;
    std::unordered_map<int, std::unique_ptr<GameRoom>> rooms;
};