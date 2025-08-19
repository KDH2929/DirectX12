#include "GameServer.h"
#include "Message/MessageHandler.h"
#include "GameSystems/FlightSystem.h"
#include "GameSystems/WeaponSystem.h"
#include "GameSystems/DamageSystem.h"
#include "GameSystems/RespawnSystem.h"

#include <chrono>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

GameServer::GameServer() = default;
GameServer::~GameServer() { Stop(); }

bool GameServer::Start(unsigned short bindPort) {
    if (runningFlag.load(std::memory_order_acquire)) return true;

    // 메시지 핸들러 생성(의존성: GameServer*)
    messageHandler = std::make_unique<MessageHandler>(this);

    // 디스패처에 핸들러 등록
    RegisterMessageHandlers();

    // IOCP 서버 생성/초기화
    iocpServer = std::make_unique<IocpServer>(messageDispatcher);
    const char* bindAddress = "0.0.0.0";
    const int workerThreadCount = 0; // 0이면 CPU*2 기본
    if (!iocpServer->Initialize(bindAddress, bindPort, workerThreadCount)) {
        std::cerr << "[GameServer] IOCP initialization failed.\n";
        return false;
    }

    // 게임 시스템 구성 및 등록
    flightSystem = std::make_unique<FlightSystem>();
    weaponSystem = std::make_unique<WeaponSystem>();
    damageSystem = std::make_unique<DamageSystem>();
    respawnSystem = std::make_unique<RespawnSystem>();

    systemPipeline.RegisterSystem(flightSystem.get());
    systemPipeline.RegisterSystem(weaponSystem.get());
    systemPipeline.RegisterSystem(damageSystem.get());
    systemPipeline.RegisterSystem(respawnSystem.get());

    // 시스템 초기화
    if (!flightSystem->Initialize())  return false;
    if (!weaponSystem->Initialize())  return false;
    if (!damageSystem->Initialize())  return false;
    if (!respawnSystem->Initialize()) return false;

    // 게임 루프 시작
    runningFlag.store(true, std::memory_order_release);
    gameLoopThread = std::thread(&GameServer::GameLoop, this);

    std::cout << "[GameServer] Started on port " << bindPort << "\n";
    return true;
}

void GameServer::Stop() {
    if (!runningFlag.load(std::memory_order_acquire)) return;

    runningFlag.store(false, std::memory_order_release);

    if (iocpServer) {
        iocpServer->Stop();
        iocpServer.reset();
    }

    if (gameLoopThread.joinable()) {
        gameLoopThread.join();
    }

    // 시스템 종료(개별 Shutdown 호출)
    if (respawnSystem) respawnSystem->Shutdown();
    if (damageSystem)  damageSystem->Shutdown();
    if (weaponSystem)  weaponSystem->Shutdown();
    if (flightSystem)  flightSystem->Shutdown();


    messageHandler.reset();

    std::cout << "[GameServer] Stopped.\n";
}

RoomManager* GameServer::GetRoomManager() { return &roomManager; }
ClientManager* GameServer::GetClientManager() { return &clientManager; }

std::uint64_t GameServer::GetServerTimeMilliseconds() const {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void GameServer::RegisterMessageHandlers() {
    auto* handler = messageHandler.get();

    messageDispatcher.RegisterHandler(
        static_cast<int>(game::GameMessage::kLoginRequest),
        [handler](std::shared_ptr<ClientSession> session, const game::GameMessage& msg) {
            handler->HandleLogin(session, msg.login_request());
        });

    messageDispatcher.RegisterHandler(
        static_cast<int>(game::GameMessage::kJoinRoomRequest),
        [handler](std::shared_ptr<ClientSession> session, const game::GameMessage& msg) {
            handler->HandleJoinRoom(session, msg.join_room_request());
        });

    messageDispatcher.RegisterHandler(
        static_cast<int>(game::GameMessage::kChatSend),
        [handler](std::shared_ptr<ClientSession> session, const game::GameMessage& msg) {
            handler->HandleChatSend(session, msg.chat_send());
        });

    messageDispatcher.RegisterHandler(
        static_cast<int>(game::GameMessage::kMoveInput),
        [handler](std::shared_ptr<ClientSession> session, const game::GameMessage& msg) {
            handler->HandleMoveInput(session, msg.move_input());
        });

    messageDispatcher.RegisterHandler(
        static_cast<int>(game::GameMessage::kFireInput),
        [handler](std::shared_ptr<ClientSession> session, const game::GameMessage& msg) {
            handler->HandleFireInput(session, msg.fire_input());
        });

    messageDispatcher.RegisterHandler(
        static_cast<int>(game::GameMessage::kHitEvent),
        [handler](std::shared_ptr<ClientSession> session, const game::GameMessage& msg) {
            handler->HandleHitEvent(session, msg.hit_event());
        });
}

void GameServer::GameLoop() {
    using namespace std::chrono;
    constexpr double fixedStepSeconds = 1.0 / 60.0; // 60 Hz
    auto previousTime = steady_clock::now();

    duration<double> accumulator(0.0);
    const duration<double> fixedStep(fixedStepSeconds);
    constexpr int maxStepsPerFrame = 5;

    while (runningFlag.load(std::memory_order_acquire)) {
        auto now = steady_clock::now();
        accumulator += (now - previousTime);
        previousTime = now;

        int steps = 0;
        while (accumulator >= fixedStep && steps < maxStepsPerFrame) {
            // 방 목록 스냅샷으로 순회
            auto rooms = roomManager.GetAllRoomList();
            for (auto* room : rooms) {
                if (!room) continue;
                // 고정 스텝 실행(파이프라인이 systems 순서대로 FixedUpdate 호출)
                systemPipeline.FixedUpdate(*room);
            }

            accumulator -= fixedStep;
            ++steps;
        }

        std::this_thread::sleep_for(milliseconds(1)); // 과점유 방지
    }
}
