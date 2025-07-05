#include "PbrLighting.hlsli"

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct VSOutput
{
    float4 positionClip : SV_POSITION;
    float3 positionWorld : POSITION;
    float3 normalWorld : NORMAL;
    float3 tangentWorld : TANGENT;
    float2 uv : TEXCOORD;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 localPosition = float4(input.position, 1.0);
    float4 worldPosition = mul(localPosition, model);
    float4 viewPosition = mul(worldPosition, view);

    output.positionClip = mul(viewPosition, projection);
    output.positionWorld = worldPosition.xyz;
    output.normalWorld = normalize(mul(input.normal, (float3x3)modelInvTranspose));
    output.tangentWorld = normalize(mul(input.tangent, (float3x3)model));
    output.uv = input.uv;

    return output;
}