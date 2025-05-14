#pragma once

#include <DirectXMath.h>

using namespace DirectX;

constexpr int MAX_LIGHTS = 3;
constexpr int NUM_DIR_LIGHTS = 1;
constexpr int NUM_POINT_LIGHTS = 1;
constexpr int NUM_SPOT_LIGHTS = 1;


enum class LightType : int {
    Directional = 0,
    Point = 1,
    Spot = 2
};

struct Light {
    XMFLOAT3 strength = XMFLOAT3(1.0f, 1.0f, 1.0f);
    float falloffStart = 0.0f;                       // point, spot

    XMFLOAT3 direction = XMFLOAT3(0.0f, -1.0f, 0.0f); // directional, spot
    float falloffEnd = 10.0f;                        // point, spot

    XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f);   // point, spot
    float specPower = 64.0f;

    int type = static_cast<int>(LightType::Directional);
    XMFLOAT3 color = XMFLOAT3(1.0f, 1.0f, 1.0f);

    // 총 크기: 3+1 + 3+1 + 3+1 + 1+3 = 12 floats = 48 bytes
};
static_assert(sizeof(Light) % 16 == 0, "Light must be 16-byte aligned");

// 상수버퍼는 16Byte 단위로 맞춰야함