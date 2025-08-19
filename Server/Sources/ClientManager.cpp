#include "ClientManager.h"

void ClientManager::AddClient(SOCKET socketHandle, int playerId, const std::string& nickname) {
    std::lock_guard<std::mutex> lock(managerMutex);

    auto session = std::make_shared<ClientSession>(socketHandle);
    session->playerId = playerId;
    session->nickname = nickname;

    sessionsBySocket[socketHandle] = session;

    if (playerId >= 0) {
        sessionsByPlayerId[playerId] = session;
    }
}

void ClientManager::RemoveClient(SOCKET socketHandle) {
    std::lock_guard<std::mutex> lock(managerMutex);

    auto it = sessionsBySocket.find(socketHandle);
    if (it == sessionsBySocket.end()) return;

    const int playerId = it->second ? it->second->playerId : -1;
    if (playerId >= 0) {
        auto pit = sessionsByPlayerId.find(playerId);
        if (pit != sessionsByPlayerId.end() && pit->second.lock() == it->second) {
            sessionsByPlayerId.erase(pit);
        }
    }

    sessionsBySocket.erase(it);
}

std::shared_ptr<ClientSession> ClientManager::GetClientBySocket(SOCKET socketHandle) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = sessionsBySocket.find(socketHandle);
    return (it != sessionsBySocket.end()) ? it->second : nullptr;
}

std::shared_ptr<ClientSession> ClientManager::GetClientByPlayerId(int playerId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = sessionsByPlayerId.find(playerId);
    if (it == sessionsByPlayerId.end()) return nullptr;

    auto locked = it->second.lock();
    if (!locked) {
        sessionsByPlayerId.erase(it);
    }
    return locked;
}

std::vector<std::shared_ptr<ClientSession>> ClientManager::GetAllClientList() const {
    std::lock_guard<std::mutex> lock(managerMutex);
    std::vector<std::shared_ptr<ClientSession>> result;
    result.reserve(sessionsBySocket.size());
    for (auto& entry : sessionsBySocket) {
        result.push_back(entry.second);
    }
    return result;
}

std::vector<std::shared_ptr<ClientSession>> ClientManager::GetClientsInRoomList(int roomId) const {
    auto allClients = GetAllClientList();
    std::vector<std::shared_ptr<ClientSession>> filtered;
    filtered.reserve(allClients.size());
    for (auto& session : allClients) {
        if (session && session->roomId == roomId) {
            filtered.push_back(session);
        }
    }
    return filtered;
}

void ClientManager::PruneExpiredPlayerIndexLocked() {
    for (auto it = sessionsByPlayerId.begin(); it != sessionsByPlayerId.end();) {
        if (it->second.expired()) {
            it = sessionsByPlayerId.erase(it);
        }
        else {
            ++it;
        }
    }
}
