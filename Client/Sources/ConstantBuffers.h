#pragma once
    
#include "Lights/LightData.h"
#include "Lights/LightType.h"
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
    LightData lights[MAX_LIGHTS];
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



static constexpr uint32_t USE_ALBEDO_MAP = 1 << 0;
static constexpr uint32_t USE_NORMAL_MAP = 1 << 1;
static constexpr uint32_t USE_METALLIC_MAP = 1 << 2;
static constexpr uint32_t USE_ROUGHNESS_MAP = 1 << 3;


// PBR 전용 (16B 정렬)
struct CB_MaterialPBR
{
    XMFLOAT3 baseColor;
    float    metallic;

    float    specular;
    float    roughness;
    float    ambientOcclusion;
    float    emissiveIntensity;

    XMFLOAT3 emissiveColor;
    uint32_t  flags;

    float    padding[4];
};

struct CB_Global {
    float time;
    float padding[3]; // 16바이트 정렬
};


struct CB_OutlineOptions
{
    XMFLOAT2 TexelSize;     // 8 bytes
    float    EdgeThreshold; // 4 bytes
    float    EdgeThickness; // 4 bytes

    XMFLOAT3 OutlineColor;  // 12 bytes
    float    MixFactor;     // 4 bytes
};


struct CB_ToneMapping
{
    float    Exposure;
    float    Gamma;
    XMFLOAT2 _pad;
};


struct CB_ShadowMapPass {
    XMMATRIX modelWorld;
    XMMATRIX lightViewProj;
};


struct CB_CloudParameters {
    XMFLOAT3 VolumeAABBMin;
    float    _pad0;
    XMFLOAT3 VolumeAABBMax;
    float    _pad1;
    XMFLOAT3 CameraPosition;
    float    StepSize;
    XMFLOAT3 SunDirection;
    float    NoiseFrequency;
    XMFLOAT4X4 InverseViewProj;
};
