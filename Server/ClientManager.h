#pragma once

#include <unordered_map>
#include <vector>
#include <mutex>
#include <algorithm>
#include <winsock2.h>
#include <string>


struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
};

// 클라이언트 개별 정보 구조체
struct ClientInfo {
    SOCKET socket;
    int playerId = -1;
    int roomId = -1;
    std::string nickname;
    Vector3 position;
    float hp = 100.0f;
};

class ClientManager {
public:
    void AddClient(SOCKET sock, int playerId = -1, const std::string& nickname = "");
    void RemoveClient(SOCKET sock);
    ClientInfo* GetClient(SOCKET sock);
    std::vector<ClientInfo*> GetAllClients();
    std::vector<ClientInfo*> GetClientsInRoom(int roomId);

private:
    std::mutex mutex;
    std::unordered_map<SOCKET, ClientInfo> clients;
};
