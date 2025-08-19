#pragma once
#include "GameSystem.h"
#include <vector>
#include <mutex>
#include <cstdint>

class GameRoom;

class WeaponSystem : public GameSystem {
public:
    const char* GetName() const noexcept override;
    int GetOrder() const noexcept override;

    void FixedUpdate(double fixedDeltaSeconds, GameRoom& room) override;
    void Update(double deltaSeconds, GameRoom& room) override; // 현재 미사용

    void RequestFire(std::uint32_t playerId, std::uint64_t serverTimeMillis);

private:
    struct FireRequest {
        std::uint32_t playerId;
        std::uint64_t timeMillis;
    };
    std::mutex queueMutex;
    std::vector<FireRequest> queued;
};