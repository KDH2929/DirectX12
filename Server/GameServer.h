#pragma once

#include <winsock2.h>
#include <thread>
#include <atomic>

#include "packet.pb.h"
#include "PacketParser.h"
#include "MessageDispatcher.h"

#include "ClientManager.h"
#include "RoomManager.h"


class GameServer {
public:
    bool Start(int port);
    void Stop();

    RoomManager* GetRoomManager();
    ClientManager* GetClientManager();


private:
    SOCKET serverSocket = INVALID_SOCKET;
    std::thread acceptThread;
    std::thread gameLoopThread;
    std::atomic<bool> running = false;

    ClientManager clientManager;
    RoomManager roomManager;
    MessageDispatcher dispatcher;

    void AcceptLoop();
    void HandleClient(SOCKET clientSocket);
    void GameLoop();

};