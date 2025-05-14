#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <winsock2.h>

#include "GameRoom.h"

class RoomManager {
public:
    GameRoom* GetRoom(int roomId);
    GameRoom& CreateRoom(int roomId);
    void RemovePlayerFromAllRooms(SOCKET sock);
    std::vector<GameRoom*> GetAllRooms();

private:
    std::mutex mutex;
    std::unordered_map<int, std::unique_ptr<GameRoom>> rooms;
};
