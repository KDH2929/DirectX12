#pragma once
#include "GameSystem.h"
#include <vector>
#include <mutex>
#include <cstdint>

class GameRoom;

class DamageSystem : public GameSystem {
public:
    const char* GetName() const noexcept override;
    int GetOrder() const noexcept override;

    // 고정 스텝 처리(대미지 적용 등)
    void FixedUpdate(double fixedDeltaSeconds, GameRoom& room) override;

    // 가변 스텝(현재 미사용)
    void Update(double deltaSeconds, GameRoom& room) override;

    struct DamageRequest {
        std::uint32_t attackerPlayerId;
        std::uint64_t targetObjectId;
        float damage;
        std::uint64_t timeMillis;
    };

    void EnqueueDamage(DamageRequest request);

private:
    std::mutex queueMutex;
    std::vector<DamageRequest> queued;
};