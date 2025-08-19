#include "GameRoom.h"
#include "ClientSession.h"
#include "Message/game_message.pb.h"
#include <iostream>

GameRoom::GameRoom(int roomIdValue)
    : roomId(roomIdValue) {}

void GameRoom::AddPlayer(ClientSession* clientSession) {
    std::lock_guard<std::mutex> lock(roomMutex);
    players.insert(clientSession);
    clientSession->roomId = roomId;
    std::cout << "[Room " << roomId << "] " << clientSession->nickname << " joined.\n";
}

void GameRoom::RemovePlayer(ClientSession* clientSession) {
    std::lock_guard<std::mutex> lock(roomMutex);
    players.erase(clientSession);
    clientSession->roomId = -1;
    std::cout << "[Room " << roomId << "] " << clientSession->nickname << " left.\n";
}

void GameRoom::BroadcastGameMessage(const game::GameMessage& message,
    const ClientSession* excludedSession)
{
    std::vector<ClientSession*> targets;
    {
        std::lock_guard<std::mutex> lock(roomMutex);
        targets.reserve(players.size());
        for (ClientSession* player : players) {
            if (player != excludedSession) targets.push_back(player);
        }
    }
    // Lock 해제 후 전송
    for (ClientSession* player : targets) {
        player->SendGameMessage(message);
    }
}

std::vector<ClientSession*> GameRoom::GetPlayerList() const {
    std::vector<ClientSession*> result;
    {
        std::lock_guard<std::mutex> lock(roomMutex);
        result.reserve(players.size());
        for (ClientSession* player : players) result.push_back(player);
    }
    return result;
}

int GameRoom::GetPlayerCount() const {
    std::lock_guard<std::mutex> lock(roomMutex);
    return static_cast<int>(players.size());
}
