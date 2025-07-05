#include "PhongLighting.hlsli"

Texture2D diffuseMap : register(t0);  // UI 텍스처
SamplerState samp : register(s0);     // 샘플러 상태

struct PSInput {
    float4 pos : SV_POSITION;   // 화면 좌표
    float2 uv : TEXCOORD;       // 텍스처 좌표
};

float4 PSMain(PSInput input) : SV_Target
{
    float4 texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

    if (material.useAlbedoMap == 1) {
        texColor = diffuseMap.Sample(samp, input.uv);
    }
    else {
        texColor.rgb = material.diffuse;
    }

    // 알파값을 고려하여 픽셀을 제거 (투명도 처리)
    clip(texColor.a - 0.5f);  // 알파값이 0.5 미만이면 픽셀 제거

    return texColor;  // 최종 색상 반환
}