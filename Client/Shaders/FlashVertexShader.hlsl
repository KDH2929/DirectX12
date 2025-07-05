#include "PhongLighting.hlsli"

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 world = mul(float4(input.position, 1.0f), model);
    output.pos = mul(mul(world, view), projection);
    output.uv = input.uv;
    return output;
}
