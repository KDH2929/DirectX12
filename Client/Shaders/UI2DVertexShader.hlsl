#include "PhongLighting.hlsli"

struct VSInput {
    float3 position : POSITION;   // ��ġ
    float3 normal : NORMAL;     // ��������� ������ �ϰ����� ���� �ʿ�
    float2 uv : TEXCOORD;         // �ؽ�ó ��ǥ
};

struct PSInput {
    float4 pos : SV_POSITION;     // ȭ�� ��ǥ
    float2 uv : TEXCOORD;         // �ؽ�ó ��ǥ
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    // �� ��ȯ �� ���� ����
    float4 world = mul(float4(input.position, 1.0f), model);  // �� ��ȯ
    output.pos = mul(mul(world, view), projection);  // �� �� �������� ��ȯ
    output.uv = input.uv;  // �ؽ�ó ��ǥ �״�� ����

    return output;
}
