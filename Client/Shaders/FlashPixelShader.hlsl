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

    // ���� �Ӱ谪 ������ ��� �ȼ� ����
    clip(texColor.a - 0.5f);

    return float4(texColor.rgb * material.alpha, texColor.a);
}
