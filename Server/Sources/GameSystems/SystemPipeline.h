#pragma once
#include <vector>

class GameRoom;
class GameSystem;

// 모든 시스템을 등록/호출하는 파이프라인(고정 스텝/가변 스텝)
class SystemPipeline {
public:
    explicit SystemPipeline(double fixedStepSeconds);

    void RegisterSystem(GameSystem* system);   // system 을 systems 컨테이너에 등록
    void FixedUpdate(GameRoom& room);          // 모든 시스템의 FixedUpdate(fixedStepSeconds) 호출
    void Update(GameRoom& room, double deltaSeconds); // 모든 시스템의 Update(deltaSeconds) 호출

private:
    double fixedStepSeconds;
    std::vector<GameSystem*> systems;
};