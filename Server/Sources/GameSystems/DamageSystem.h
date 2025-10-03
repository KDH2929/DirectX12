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

    // ���� ���� ó��(����� ���� ��)
    void FixedUpdate(double fixedDeltaSeconds, GameRoom& room) override;

    // ���� ����(���� �̻��)
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