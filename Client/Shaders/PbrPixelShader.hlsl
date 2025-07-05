#include "PbrLighting.hlsli"

struct VSOutput
{
    float4 positionClip : SV_POSITION;
    float3 positionWorld : POSITION;
    float3 normalWorld : NORMAL;
    float3 tangentWorld : TANGENT;
    float2 uv : TEXCOORD;
};

float3x3 TBNMatrix(float3 T, float3 B, float3 N)
{
    return float3x3(T, B, N);
}

float4 PSMain(VSOutput input) : SV_Target
{
    float3 tangentNormal = SampleNormal(input.uv);
    float3 N = normalize(input.normalWorld);
    float3 T = normalize(input.tangentWorld);
    float3 B = normalize(cross(N, T));
    float3 normalWorld = normalize(mul(tangentNormal, TBNMatrix(T, B, N)));

    float3 color = ComputePBR(input.positionWorld, normalWorld, input.uv);
    return float4(color, 1);
    
}