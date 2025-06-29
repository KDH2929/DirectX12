#include "Common.hlsli"

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PixelInput VSMain(VertexInput input)
{
    PixelInput output;

    // 월드·뷰·투영 변환
    float4 worldPos = mul(float4(input.position, 1.0f), model);
    float4 viewPos = mul(worldPos, view);
    output.position = mul(viewPos, projection);

    output.color = input.color;
    return output;
}