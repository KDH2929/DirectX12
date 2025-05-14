#include "GameRoom.h"
#include <algorithm>

GameRoom::GameRoom(int roomId) : roomId(roomId) {}

void GameRoom::AddPlayer(SOCKET sock) {
    std::lock_guard<std::mutex> lock(mutex);
    players.push_back(sock);
}

void GameRoom::RemovePlayer(SOCKET sock) {
    std::lock_guard<std::mutex> lock(mutex);
    players.erase(std::remove(players.begin(), players.end(), sock), players.end());
}

void GameRoom::Broadcast(const char* data, int size, SOCKET excludeSock) {
    std::lock_guard<std::mutex> lock(mutex);
    for (SOCKET player : players) {
        if (player != excludeSock) {
            send(player, data, size, 0);
        }
    }
}

int GameRoom::GetRoomId() const {
    return roomId;
}

const std::vector<SOCKET>& GameRoom::GetPlayers() const {
    return players;
}
