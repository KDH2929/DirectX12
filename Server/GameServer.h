#pragma once

#include <winsock2.h>
#include <thread>
#include <atomic>

#include "ClientManager.h"
#include "RoomManager.h"


class GameServer {
public:
    bool Start(int port);
    void Stop();

private:
    SOCKET serverSocket = INVALID_SOCKET;
    std::thread acceptThread;
    std::thread gameLoopThread;
    std::atomic<bool> running = false;

    ClientManager clientManager;
    RoomManager roomManager;

    void AcceptLoop();
    void HandleClient(SOCKET clientSocket);
    void GameLoop();
};