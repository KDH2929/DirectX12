#pragma once

#include <unordered_set>
#include <mutex>
#include "ClientSession.h"

class GameRoom {
public:
    explicit GameRoom(int roomId);

    void AddPlayer(ClientSession* client);
    void RemovePlayer(ClientSession* client);
    void Broadcast(const game::Packet& packet, ClientSession* excludeClient = nullptr);

    int GetRoomId() const;
    const std::unordered_set<ClientSession*>& GetPlayers() const;
    int GetPlayerCount() const;

private:
    int roomId;
    std::unordered_set<ClientSession*> players;
    mutable std::mutex mutex;
};