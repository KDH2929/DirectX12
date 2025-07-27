#pragma once

#define MAX_LIGHTS 3
#define NUM_DIR_LIGHTS 1
#define NUM_POINT_LIGHTS 1
#define NUM_SPOT_LIGHTS 1

#define MAX_SHADOW_DSV_COUNT (NUM_DIR_LIGHTS + NUM_SPOT_LIGHTS + 6 * NUM_POINT_LIGHTS)
#define SHADOW_MAP_WIDTH 2048
#define SHADOW_MAP_HEIGHT 2048

static const float INV_SHADOW_MAP_WIDTH = 1.0f / SHADOW_MAP_WIDTH;
static const float INV_SHADOW_MAP_HEIGHT = 1.0f / SHADOW_MAP_HEIGHT;;


struct Light
{
    float3 Position;
    float _pad0;

    float3 Direction;
    float _pad1;

    float3 Color;
    float Strength;

    float InnerCutoffAngle;
    float OuterCutoffAngle;
    int ShadowCastingEnabled;
    float _pad3;

    float Constant;
    float Linear;
    float Quadratic;
    int Type;

};