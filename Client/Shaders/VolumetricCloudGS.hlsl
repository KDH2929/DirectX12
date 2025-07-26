#include "CloudCommon.hlsli"


// ���� ť�����·� Ȯ��
struct VSOutput
{
    float4 PositionClip : SV_POSITION;
    float3 PositionWorld : TEXCOORD0;
};

[maxvertexcount(36)]
void GSMain(point VSOutput input[1], inout TriangleStream<VSOutput> triStream)
{
    // ���� AABB�� 8�� �ڳ�
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

    // 12�� �� 2�ﰢ�� = 36 �ε���
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
        // ���� �� ���� ��ȯ
        float3 worldPos = mul(float4(corners[indices[i]], 1.0f), model).xyz;

        VSOutput o;
        o.PositionWorld = worldPos;
        // ���� �� Ŭ�� ����
        o.PositionClip = mul(float4(worldPos, 1.0f), viewProj);

        triStream.Append(o);
    }
    // triangle list ����̹Ƿ� RestartStrip() ���ʿ�
}