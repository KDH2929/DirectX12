#pragma once
#include <cstdint>

struct Plane {
    std::uint64_t id = 0;       // object_id(�������� ��ȣ���)
    std::uint32_t playerId = 0; // ���� �ĺ���

    // ��ġ/�ӵ�(���� ��ǥ)
    float px = 0, py = 0, pz = 0;
    float vx = 0, vy = 0, vz = 0;

    // �ڼ�(���ʹϾ�)
    float qw = 1, qx = 0, qy = 0, qz = 0;

    // ���ӵ�(rad/s)
    float wx = 0, wy = 0, wz = 0;

    // �Է�(������ �ֽ� �Է� ����)
    float inputPitch = 0;   // -1..+1
    float inputRoll = 0;    // -1..+1
    float inputYaw = 0;     // -1..+1
    float inputThrottle = 0; // 0..1

    // ���� �Ķ����(���� �⺻��)
    float massKg = 3000.0f;
    float wingArea = 20.0f;
    float cl0 = 0.2f, cla = 5.5f; // ��� ���
    float cd0 = 0.02f, k = 0.04f; // �׷� ���
    float maxThrust = 30000.0f;   // N

    float hitPoints = 100.0f;
    bool alive = true;
};
