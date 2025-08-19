#pragma once
#include "GameSystem.h"
#include <vector>
#include <mutex>
#include <cstdint>

class GameRoom;

class RespawnSystem : public GameSystem {
public:
    const char* GetName() const noexcept override;
    int GetOrder() const noexcept override;

    void FixedUpdate(double fixedDeltaSeconds, GameRoom& room) override;
    void Update(double deltaSeconds, GameRoom& room) override; // 현재 미사용

    void RequestRespawn(std::uint32_t playerId, double delaySeconds);

private:
    struct RespawnTask {
        std::uint32_t playerId;
        double remainingSeconds;
    };
    std::mutex listMutex;
    std::vector<RespawnTask> waiting;
};