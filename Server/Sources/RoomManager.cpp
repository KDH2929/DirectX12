#include "RoomManager.h"
#include "Message/game_message.pb.h"  // 구현부에서만 포함
#include <vector>

GameRoom* RoomManager::GetRoom(int roomId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = roomsById.find(roomId);
    return (it != roomsById.end()) ? it->second.get() : nullptr;
}

const GameRoom* RoomManager::GetRoom(int roomId) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = roomsById.find(roomId);
    return (it != roomsById.end()) ? it->second.get() : nullptr;
}

GameRoom& RoomManager::GetOrCreateRoom(int roomId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = roomsById.find(roomId);
    if (it == roomsById.end()) {
        roomsById[roomId] = std::make_unique<GameRoom>(roomId);
    }
    return *roomsById[roomId];
}

void RoomManager::BroadcastGameMessageToRoom(int roomId,
    const game::GameMessage& message,
    const ClientSession* excludedSession)
{
    GameRoom* targetRoom = nullptr;
    {
        std::lock_guard<std::mutex> lock(managerMutex);
        auto it = roomsById.find(roomId);
        if (it != roomsById.end()) {
            targetRoom = it->second.get();
        }
    }
    if (targetRoom) {
        targetRoom->BroadcastGameMessage(message, excludedSession);
    }
}

void RoomManager::RemovePlayerFromAllRooms(ClientSession* clientSession) {
    std::vector<GameRoom*> roomList;
    {
        std::lock_guard<std::mutex> lock(managerMutex);
        roomList.reserve(roomsById.size());
        for (auto& entry : roomsById) {
            roomList.push_back(entry.second.get());
        }
    }
    for (GameRoom* room : roomList) {
        if (room) room->RemovePlayer(clientSession);
    }
}

std::vector<GameRoom*> RoomManager::GetAllRoomList() {
    std::lock_guard<std::mutex> lock(managerMutex);
    std::vector<GameRoom*> roomList;
    roomList.reserve(roomsById.size());
    for (auto& entry : roomsById) {
        roomList.push_back(entry.second.get());
    }
    return roomList;
}

std::vector<const GameRoom*> RoomManager::GetAllRoomList() const {
    std::lock_guard<std::mutex> lock(managerMutex);
    std::vector<const GameRoom*> roomList;
    roomList.reserve(roomsById.size());
    for (auto& entry : roomsById) {
        roomList.push_back(entry.second.get());
    }
    return roomList;
}