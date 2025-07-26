#include "PhongLighting.hlsli"

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 posWorld : POSITION;
    float3 normalWorld : NORMAL;
    float3 tangentWorld : TANGENT;
    float2 uv : TEXCOORD;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 localPosition = float4(input.position, 1.0f);
    float4 worldPosition = mul(localPosition, model);
    float4 viewPosition = mul(worldPosition, view);
    output.pos = mul(viewPosition, projection);

    output.posWorld = worldPosition.xyz;
    output.normalWorld = normalize(mul(input.normal, (float3x3)modelInvTranspose));
    output.tangentWorld = normalize(mul(input.tangent, (float3x3)model));
    output.uv = input.uv;

    return output;
}