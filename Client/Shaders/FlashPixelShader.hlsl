#include "PhongLighting.hlsli"

Texture2D diffuseMap : register(t0);
SamplerState samp : register(s0);


struct PSInput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 PSMain(PSInput input) : SV_Target
{
    float4 texColor = diffuseMap.Sample(samp, input.uv);

    // 투명도 임계값 이하일 경우 픽셀 제거
    clip(texColor.a - 0.5f);

    return float4(texColor.rgb * material.alpha, texColor.a);
}
