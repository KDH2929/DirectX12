#include "Common.hlsli"

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float3 posWorld : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};


VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 localPos = float4(input.position, 1.0f);
    float4 worldPos = mul(localPos, model);
    float4 viewPos = mul(worldPos, view);
    float4 projPos = mul(viewPos, projection);

    float3 worldNormal = normalize(mul(input.normal, (float3x3)modelInvTranspose));

    output.pos = projPos;
    output.posWorld = worldPos.xyz;
    output.normal = worldNormal;
    output.uv = input.uv;

    return output;
}