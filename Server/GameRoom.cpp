#include "GameRoom.h"
#include <iostream>

GameRoom::GameRoom(int roomId) : roomId(roomId) {}

void GameRoom::AddPlayer(ClientSession* client) {
    std::lock_guard<std::mutex> lock(mutex);
    players.insert(client);
    client->roomId = roomId;
    std::cout << "[Room " << roomId << "] " << client->nickname << " joined.\n";
}

void GameRoom::RemovePlayer(ClientSession* client) {
    std::lock_guard<std::mutex> lock(mutex);
    players.erase(client);
    std::cout << "[Room " << roomId << "] " << client->nickname << " left.\n";
}

void GameRoom::Broadcast(const game::Packet& packet, ClientSession* excludeClient) {
    std::lock_guard<std::mutex> lock(mutex);
    for (ClientSession* player : players) {
        if (player != excludeClient) {
            player->SendPacket(packet);
        }
    }
}

int GameRoom::GetRoomId() const {
    return roomId;
}

const std::unordered_set<ClientSession*>& GameRoom::GetPlayers() const {
    return players;
}

int GameRoom::GetPlayerCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return static_cast<int>(players.size());
}