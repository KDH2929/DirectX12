#pragma once

#include <DirectXMath.h>
using namespace DirectX;

struct LightData
{
    XMFLOAT3 strength;
    float falloffStart;

    XMFLOAT3 direction;
    float falloffEnd;

    XMFLOAT3 position;
    float specPower;

    int type;
    XMFLOAT3 color;
};
static_assert(sizeof(LightData) % 16 == 0, "LightData must be 16-byte aligned");
