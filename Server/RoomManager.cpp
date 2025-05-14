#include "RoomManager.h"

GameRoom* RoomManager::GetRoom(int roomId)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        return it->second.get();        // unique_ptr에서 포인터 추출
    }
    return nullptr;
}

GameRoom& RoomManager::CreateRoom(int roomId)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto it = rooms.find(roomId);
    if (it == rooms.end()) {
        rooms[roomId] = std::make_unique<GameRoom>(roomId);
    }

    return *rooms[roomId];
}

void RoomManager::RemovePlayerFromAllRooms(SOCKET sock) {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        it->second->RemovePlayer(sock);
    }
}

std::vector<GameRoom*> RoomManager::GetAllRooms() {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<GameRoom*> result;
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        result.push_back(it->second.get());
    }
    return result;
}
