#pragma once

#include <vector>
#include <mutex>
#include <winsock2.h>


class GameRoom {
public:
    explicit GameRoom(int roomId);

    void AddPlayer(SOCKET sock);
    void RemovePlayer(SOCKET sock);
    void Broadcast(const char* data, int size, SOCKET excludeSock = INVALID_SOCKET);

    int GetRoomId() const;
    const std::vector<SOCKET>& GetPlayers() const;

private:
    int roomId;
    std::vector<SOCKET> players;
    std::mutex mutex;
};
