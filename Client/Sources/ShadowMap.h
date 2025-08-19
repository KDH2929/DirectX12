#pragma once

#include <wrl/client.h>
#include <d3d12.h>

#include "DescriptorHandle.h"
#include "Lights/LightType.h"

using Microsoft::WRL::ComPtr;

constexpr UINT SHADOW_MAP_WIDTH = 2048;
constexpr UINT SHADOW_MAP_HEIGHT = 2048;

constexpr INT MAX_SHADOW_DSV_COUNT = NUM_DIR_LIGHTS + NUM_SPOT_LIGHTS + 6 * NUM_POINT_LIGHTS;

struct ShadowMap
{
    ComPtr<ID3D12Resource> depthBuffer;
    DescriptorHandle dsvHandle; // CPU Handle만 필요
    DescriptorHandle srvHandle;  // CPU + GPU 둘 다 필요
};