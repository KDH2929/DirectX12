#pragma once
#include "GameSystem.h"

class GameRoom;

class FlightSystem : public GameSystem {
public:
    const char* GetName() const noexcept override;
    int GetOrder() const noexcept override;

    void FixedUpdate(double fixedDeltaSeconds, GameRoom& room) override;
    void Update(double deltaSeconds, GameRoom& room) override; // 현재 미사용
};