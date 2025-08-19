#pragma once
#include <cstdint>

struct Plane {
    std::uint64_t id = 0;       // object_id(프로토콜 상호운용)
    std::uint32_t playerId = 0; // 세션 식별자

    // 위치/속도(월드 좌표)
    float px = 0, py = 0, pz = 0;
    float vx = 0, vy = 0, vz = 0;

    // 자세(쿼터니언)
    float qw = 1, qx = 0, qy = 0, qz = 0;

    // 각속도(rad/s)
    float wx = 0, wy = 0, wz = 0;

    // 입력(서버가 최신 입력 보관)
    float inputPitch = 0;   // -1..+1
    float inputRoll = 0;    // -1..+1
    float inputYaw = 0;     // -1..+1
    float inputThrottle = 0; // 0..1

    // 성능 파라미터(간단 기본값)
    float massKg = 3000.0f;
    float wingArea = 20.0f;
    float cl0 = 0.2f, cla = 5.5f; // 양력 계수
    float cd0 = 0.02f, k = 0.04f; // 항력 계수
    float maxThrust = 30000.0f;   // N

    float hitPoints = 100.0f;
    bool alive = true;
};
