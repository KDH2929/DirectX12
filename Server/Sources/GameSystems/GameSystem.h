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

    // ���� ����(�ù�/����/�̵�/���� ��)
    virtual void FixedUpdate(double fixedDeltaSeconds, GameRoom& room) = 0;

    // ���� ����(�α�, ��ù� �۾� ��)
    virtual void Update(double deltaSeconds, GameRoom& room) {}
};
