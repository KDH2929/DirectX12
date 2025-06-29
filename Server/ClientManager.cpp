#include "ClientManager.h"

void ClientManager::AddClient(SOCKET sock, int playerId, const std::string& nickname) {
    std::lock_guard<std::mutex> lock(mutex);
    auto client = std::make_shared<ClientSession>(sock);
    client->playerId = playerId;
    client->nickname = nickname;
    clients[sock] = client;
}

void ClientManager::RemoveClient(SOCKET sock) {
    std::lock_guard<std::mutex> lock(mutex);
    clients.erase(sock);
}

std::shared_ptr<ClientSession> ClientManager::GetClient(SOCKET sock) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = clients.find(sock);
    if (it != clients.end()) {
        return it->second;
    }
    return nullptr;
}

ClientSession* ClientManager::GetClientById(int playerId) {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->playerId == playerId) {
            return it->second.get();
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<ClientSession>> ClientManager::GetAllClients() {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<std::shared_ptr<ClientSession>> result;
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

std::vector<std::shared_ptr<ClientSession>> ClientManager::GetClientsInRoom(int roomId) {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<std::shared_ptr<ClientSession>> result;
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->roomId == roomId) {
            result.push_back(it->second);
        }
    }
    return result;
}
