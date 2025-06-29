#include "RoomManager.h"

GameRoom* RoomManager::GetRoom(int roomId) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        return it->second.get();
    }
    return nullptr;
}

GameRoom& RoomManager::GetOrCreateRoom(int roomId) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = rooms.find(roomId);
    if (it == rooms.end()) {
        rooms[roomId] = std::make_unique<GameRoom>(roomId);
    }

    return *rooms[roomId];
}

void RoomManager::BroadcastToRoom(int roomId, const game::Packet& packet, ClientSession* excludeClient) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        it->second->Broadcast(packet, excludeClient);
    }
}
void RoomManager::RemovePlayerFromAllRooms(ClientSession* client) {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& pair : rooms) {
        pair.second->RemovePlayer(client);
    }
}

std::vector<GameRoom*> RoomManager::GetAllRooms() {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<GameRoom*> result;
    for (auto& pair : rooms) {
        result.push_back(pair.second.get());
    }
    return result;
}
