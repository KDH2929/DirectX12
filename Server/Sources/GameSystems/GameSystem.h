#pragma once
#include <string>

class GameRoom;

class GameSystem {
public:
    virtual ~GameSystem() = default;

    virtual const char* GetName() const noexcept = 0;
    virtual int GetOrder() const noexcept = 0;

    virtual bool Initialize() { return true; }
    virtual void Shutdown() {}

    // 고정 스텝(시뮬/전투/이동/물리 등)
    virtual void FixedUpdate(double fixedDeltaSeconds, GameRoom& room) = 0;

    // 가변 스텝(로그, 비시뮬 작업 등)
    virtual void Update(double deltaSeconds, GameRoom& room) {}
};
