#include "ClientManager.h"

void ClientManager::AddClient(SOCKET sock, int playerId, const std::string& nickname) {
    std::lock_guard<std::mutex> lock(mutex);
    ClientInfo info;
    info.socket = sock;
    info.playerId = playerId;
    info.nickname = nickname;
    clients[sock] = info;
}

void ClientManager::RemoveClient(SOCKET sock) {
    std::lock_guard<std::mutex> lock(mutex);
    clients.erase(sock);
}

ClientInfo* ClientManager::GetClient(SOCKET sock) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = clients.find(sock);
    if (it != clients.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<ClientInfo*> ClientManager::GetAllClients() {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<ClientInfo*> result;
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        result.push_back(&it->second);
    }
    return result;
}

std::vector<ClientInfo*> ClientManager::GetClientsInRoom(int roomId) {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<ClientInfo*> result;
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->second.roomId == roomId) {
            result.push_back(&it->second);
        }
    }
    return result;
}
