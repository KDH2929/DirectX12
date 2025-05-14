#include "Common.hlsli"

Texture2D diffuseMap : register(t0);
SamplerState samplerState : register(s0);

struct PSInput {
    float4 pos : SV_POSITION;
    float3 posWorld : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

float4 PSMain(PSInput input) : SV_Target
{
    float3 normal = normalize(input.normal);
    float3 toEye = normalize(cameraWorld - input.posWorld);

    float4 texColor = diffuseMap.Sample(samplerState, input.uv);

    Material mat = material;

    if (mat.useTexture != 0)
        mat.diffuse *= texColor.rgb;

    float3 finalColor = mat.ambient;

    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        finalColor += ComputeLight(lights[i], mat, input.posWorld, normal, toEye);
    }

    return float4(finalColor, 1.0f);
}
