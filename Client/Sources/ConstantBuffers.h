#pragma once

#include "Light.h"
#include <DirectXMath.h>

using namespace DirectX;

// b0
struct CB_MVP
{
    XMFLOAT4X4 model;
    XMFLOAT4X4 view;
    XMFLOAT4X4 projection;
    XMFLOAT4X4 modelInvTranspose;
};

// b1
struct CB_Lighting
{
    XMFLOAT3 cameraWorld;
    float padding; // align 16 bytes
    Light lights[MAX_LIGHTS];
};


// b2
struct CB_Material
{
    XMFLOAT3 ambient;
    float padding0;

    XMFLOAT3 diffuse;
    float padding1;

    XMFLOAT3 specular;
    float alpha;

    uint32_t useAlbedoMap;
    uint32_t useNormalMap;
    XMFLOAT2 padding2;        // align 16
};


// b3
struct CB_Global {
    float time;
    float padding[3]; // 16바이트 정렬
};