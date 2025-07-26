#include "CloudCommon.hlsli"


// 점을 큐브형태로 확장
struct VSOutput
{
    float4 PositionClip : SV_POSITION;
    float3 PositionWorld : TEXCOORD0;
};

[maxvertexcount(36)]
void GSMain(point VSOutput input[1], inout TriangleStream<VSOutput> triStream)
{
    // 로컬 AABB의 8개 코너
    float3 corners[8] = {
        float3(VolumeAABBMin.x, VolumeAABBMin.y, VolumeAABBMin.z),
        float3(VolumeAABBMax.x, VolumeAABBMin.y, VolumeAABBMin.z),
        float3(VolumeAABBMax.x, VolumeAABBMax.y, VolumeAABBMin.z),
        float3(VolumeAABBMin.x, VolumeAABBMax.y, VolumeAABBMin.z),
        float3(VolumeAABBMin.x, VolumeAABBMin.y, VolumeAABBMax.z),
        float3(VolumeAABBMax.x, VolumeAABBMin.y, VolumeAABBMax.z),
        float3(VolumeAABBMax.x, VolumeAABBMax.y, VolumeAABBMax.z),
        float3(VolumeAABBMin.x, VolumeAABBMax.y, VolumeAABBMax.z)
    };

    // 12면 × 2삼각형 = 36 인덱스
    int indices[36] = {
         0,1,2, 0,2,3,
         4,6,5, 4,7,6,
         4,5,1, 4,1,0,
         3,2,6, 3,6,7,
         1,5,6, 1,6,2,
         4,0,3, 4,3,7
    };

    float4x4 viewProj = mul(view, projection);

    for (int i = 0; i < 36; ++i)
    {
        // 로컬 → 월드 변환
        float3 worldPos = mul(float4(corners[indices[i]], 1.0f), model).xyz;

        VSOutput o;
        o.PositionWorld = worldPos;
        // 월드 → 클립 공간
        o.PositionClip = mul(float4(worldPos, 1.0f), viewProj);

        triStream.Append(o);
    }
    // triangle list 모드이므로 RestartStrip() 불필요
}