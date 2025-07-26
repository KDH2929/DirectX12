#include "CloudCommon.hlsli"

struct VSOutput
{
    float4 PositionClip : SV_POSITION;
    float3 PositionWorld : TEXCOORD0;
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput o;
    o.PositionClip = float4(0, 0, 0, 1);
    o.PositionWorld = float3(0, 0, 0);
    return o;
}
