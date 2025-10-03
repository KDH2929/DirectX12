#pragma once
#include <vector>

class GameRoom;
class GameSystem;

// ��� �ý����� ���/ȣ���ϴ� ����������(���� ����/���� ����)
class SystemPipeline {
public:
    explicit SystemPipeline(double fixedStepSeconds);

    void RegisterSystem(GameSystem* system);   // system �� systems �����̳ʿ� ���
    void FixedUpdate(GameRoom& room);          // ��� �ý����� FixedUpdate(fixedStepSeconds) ȣ��
    void Update(GameRoom& room, double deltaSeconds); // ��� �ý����� Update(deltaSeconds) ȣ��

private:
    double fixedStepSeconds;
    std::vector<GameSystem*> systems;
};