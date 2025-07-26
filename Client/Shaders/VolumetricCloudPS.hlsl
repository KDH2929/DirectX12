#include "CloudCommon.hlsli"
#include "Noise3D.hlsli"

struct PSInput
{
    float4 PositionClip : SV_POSITION;  // 클립 공간 위치 (w 포함)
    float3 PositionWorld : TEXCOORD0;   // GS 에서 넘겨준 월드 좌표
};

float4 PSMain(PSInput input) : SV_Target
{
    // 1) SV_POSITION → NDC(-1..+1) → UV(0..1)
    float2 ndc = input.PositionClip.xy / input.PositionClip.w;
    float2 uv = ndc * 0.5f + 0.5f;

    // 2) 카메라 공간에서 광선 계산
    float4 homogenous = float4(uv * 2.0f - 1.0f, 1.0f, 1.0f);
    float4 worldRay4 = mul(homogenous, InverseViewProj);
    float3 rayDir = normalize(worldRay4.xyz / worldRay4.w - CameraPosition);

    // 3) 레이 마칭
    float3 samplePos = CameraPosition;
    float  alphaAccum = 0.0f;
    float3 colorAccum = float3(0,0,0);

    [unroll]
    for (int i = 0; i < 64; ++i)
    {
        samplePos += rayDir * StepSize;

        // AABB 밖으로 나가면 종료
        if (any(samplePos < VolumeAABBMin) || any(samplePos > VolumeAABBMax))
            break;

        float noiseValue = PerlinNoise3D(samplePos * NoiseFrequency);
        noiseValue = saturate(noiseValue * 0.5 + 0.5);

        float lightFactor = saturate(dot(SunDirection, rayDir));
        float3 sampleColor = lerp(float3(1,1,1), float3(0.7,0.8,1), noiseValue) * lightFactor;

        float alphaStep = noiseValue * 0.05;
        colorAccum += (1 - alphaAccum) * sampleColor * alphaStep;
        alphaAccum += (1 - alphaAccum) * alphaStep;
        if (alphaAccum >= 0.95)
            break;
    }

    return float4(colorAccum, alphaAccum);
}
