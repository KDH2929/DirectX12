cbuffer CB_MVP : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
};

struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float3 direction : TEXCOORD;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(float4(input.position, 1.0f), model);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, projection);
    output.direction = input.position;  // sample direction
    return output;
}