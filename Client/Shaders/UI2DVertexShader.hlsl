#include "Common.hlsli"

struct VSInput {
    float3 position : POSITION;   // 위치
    float3 normal : NORMAL;     // 사용하지는 않지만 일관성을 위해 필요
    float2 uv : TEXCOORD;         // 텍스처 좌표
};

struct PSInput {
    float4 pos : SV_POSITION;     // 화면 좌표
    float2 uv : TEXCOORD;         // 텍스처 좌표
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    // 모델 변환 및 투영 적용
    float4 world = mul(float4(input.position, 1.0f), model);  // 모델 변환
    output.pos = mul(mul(world, view), projection);  // 뷰 및 프로젝션 변환
    output.uv = input.uv;  // 텍스처 좌표 그대로 전달

    return output;
}
