#include "PhongLighting.hlsli"

Texture2D diffuseMap : register(t0);  // UI �ؽ�ó
SamplerState samp : register(s0);     // ���÷� ����

struct PSInput {
    float4 pos : SV_POSITION;   // ȭ�� ��ǥ
    float2 uv : TEXCOORD;       // �ؽ�ó ��ǥ
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

    // ���İ��� ����Ͽ� �ȼ��� ���� (���� ó��)
    clip(texColor.a - 0.5f);  // ���İ��� 0.5 �̸��̸� �ȼ� ����

    return texColor;  // ���� ���� ��ȯ
}