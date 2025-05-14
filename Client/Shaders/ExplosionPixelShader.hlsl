#include "Common.hlsli"

cbuffer CB_UVRect : register(b4)
{
    float2 uvRect0;
    float2 uvRect1;
    float2 uvRect2;
    float2 uvRect3;
};

Texture2D diffuseMap : register(t0);
SamplerState samp : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 PSMain(PSInput input) : SV_Target
{
     float2 uvOffset = uvRect0;
     float2 uvSize = uvRect3 - uvRect0;
     float2 uv = uvOffset + input.uv * uvSize;

    float4 texColor = diffuseMap.Sample(samp, uv);

    clip(texColor.a - 0.5f);

    return float4(texColor.rgb, texColor.a * material.alpha);
}
