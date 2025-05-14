#include "Common.hlsli"

Texture2D diffuseMap : register(t0);
Texture2D noiseMap : register(t1);
SamplerState samp : register(s0);

struct PSInput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 PSMain(PSInput input) : SV_Target
{
    float2 uv = input.uv;

    // 노이즈 기반 UV 왜곡
    float2 noiseUV = uv + float2(time * 0.1, time * 0.1);
    float noise = noiseMap.Sample(samp, noiseUV).r;
    float2 offset = (noise - 0.5f) * 0.05f;
    float2 warpedUV = uv + offset;

    float4 texColor = diffuseMap.Sample(samp, warpedUV);

    clip(texColor.a - 0.2f); // 알파 컷오프

    float fade = material.alpha;
    float3 rgb = texColor.rgb * texColor.a * fade;
    float alpha = texColor.a * fade;
    return float4(rgb, alpha);
}