#pragma once

#include "Light.h"
#include <DirectXMath.h>

using namespace DirectX;

struct CB_MVP
{
    XMFLOAT4X4 model;
    XMFLOAT4X4 view;
    XMFLOAT4X4 projection;
    XMFLOAT4X4 modelInvTranspose;
};

struct CB_Lighting
{
    XMFLOAT3 cameraWorld;
    float padding; // align 16 bytes
    Light lights[MAX_LIGHTS];
};


struct CB_MaterialPhong
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

// PBR 전용 (16B 정렬)
struct CB_MaterialPBR
{
    XMFLOAT3 baseColor;
    float    metallic;       // 16

    float    specular;
    float    roughness;      // 24

    float    ambientOcclusion;
    float    emissiveIntensity; // 32

    XMFLOAT3 emissiveColor;
    float    pad0;           // 44 +4 =48

    // 여기부터 16바이트 단위 패딩
    float    pad1[4];        // +16 = 64바이트
};


struct CB_Global {
    float time;
    float padding[3]; // 16바이트 정렬
};