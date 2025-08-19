#pragma once

#include <atomic>
#include <thread>
#include <memory>
#include <cstdint>

#include "GameSystems/SystemPipeline.h"
#include "RoomManager.h"
#include "ClientManager.h"
#include "IOCP/IocpServer.h"
#include "Message/MessageDispatcher.h"
#include "Message/MessageHandler.h"



class FlightSystem;
class WeaponSystem;
class DamageSystem;
class RespawnSystem;


class GameServer {
public:
    GameServer();
    ~GameServer();

    bool Start(unsigned short bindPort);
    void Stop();

    RoomManager* GetRoomManager();
    ClientManager* GetClientManager();
    std::uint64_t GetServerTimeMilliseconds() const;

private:
    void RegisterMessageHandlers();
    void GameLoop();

private:
    std::atomic<bool> runningFlag{ false };
    std::thread gameLoopThread;

    RoomManager roomManager;
    ClientManager clientManager;

    MessageDispatcher messageDispatcher;
    std::unique_ptr<IocpServer> iocpServer;
    std::unique_ptr<MessageHandler> messageHandler;


    SystemPipeline systemPipeline;

    std::unique_ptr<FlightSystem>  flightSystem;
    std::unique_ptr<WeaponSystem>  weaponSystem;
    std::unique_ptr<DamageSystem>  damageSystem;
    std::unique_ptr<RespawnSystem> respawnSystem;
};
