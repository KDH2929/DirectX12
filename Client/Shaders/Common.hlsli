#pragma once

#define MAX_LIGHTS 3
#define NUM_DIR_LIGHTS 1
#define NUM_POINT_LIGHTS 1
#define NUM_SPOT_LIGHTS 1

#define MAX_SHADOW_DSV_COUNT (NUM_DIR_LIGHTS + NUM_SPOT_LIGHTS + 6 * NUM_POINT_LIGHTS)


struct Light
{
    float3 strength;
    float falloffStart;

    float3 direction;
    float falloffEnd;

    float3 position;
    float specPower;

    int type;
    float3 color;
};