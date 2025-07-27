#pragma once

#include <DirectXMath.h>
using namespace DirectX;

struct LightData
{
    XMFLOAT3 position;
    float     _pad0;

    XMFLOAT3 direction;
    float     _pad1;

    XMFLOAT3 color;
    float     strength;

    float     innerCutoffAngle;
    float     outerCutoffAngle;
    int       shadowCastingEnabled = 1;
    float     _pad3;

    float     constant;
    float     linear;
    float     quadratic;
    int       type;
};
static_assert(sizeof(LightData) % 16 == 0, "LightData must be 16-byte aligned");