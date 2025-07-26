#include "PhongLighting.hlsli"

Texture2D albedoMap : register(t0);
Texture2D normalMap : register(t1);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 posWorld : POSITION;
    float3 normalWorld : NORMAL;
    float3 tangentWorld : TANGENT;
    float2 uv : TEXCOORD;
};

float4 PSMain(PSInput input) : SV_Target
{
    Material materialData = material;

    // Albedo
    float3 albedoColor = albedoMap.Sample(samplerState, input.uv).rgb;
    if (materialData.useAlbedoMap == 0)
        albedoColor = materialData.diffuse;
    else
        albedoColor *= materialData.diffuse;

    // Normal map
    float3 normalTS = normalMap.Sample(samplerState, input.uv).xyz * 2.0 - 1.0;
    if (materialData.useNormalMap == 0)
        normalTS = float3(0, 0, 1);

    float3 normalWorld = normalize(input.normalWorld);
    float3 tangentWorld = normalize(input.tangentWorld);
    float3 bitangentWorld = normalize(cross(normalWorld, tangentWorld));
    float3x3 tbnMatrix = float3x3(tangentWorld, bitangentWorld, normalWorld);
    float3 finalNormal = normalize(mul(normalTS, tbnMatrix));

    float3 vectorToEye = normalize(cameraWorld - input.posWorld);

    float3 finalColor = materialData.ambient;

    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
        finalColor += ComputeLight(lights[i], materialData, input.posWorld, finalNormal, vectorToEye);

    return float4(finalColor * albedoColor, 1.0f);
}