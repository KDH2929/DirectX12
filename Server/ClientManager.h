#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include "ClientSession.h"

class ClientManager {
public:
    void AddClient(SOCKET sock, int playerId = -1, const std::string& nickname = "");
    void RemoveClient(SOCKET sock);

    std::shared_ptr<ClientSession> GetClient(SOCKET sock);
    ClientSession* GetClientById(int playerId);

    std::vector<std::shared_ptr<ClientSession>> GetAllClients();
    std::vector<std::shared_ptr<ClientSession>> GetClientsInRoom(int roomId);

private:
    std::mutex mutex;
    std::unordered_map<SOCKET, std::shared_ptr<ClientSession>> clients;
};