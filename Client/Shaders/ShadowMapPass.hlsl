cbuffer CB_ShadowMapPass : register(b0)
{
    float4x4 worldMatrix;
    float4x4 lightViewProjMatrix;
};

struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    output.position = mul(worldPos, lightViewProjMatrix);
    return output;
}

void PSMain(VSOutput input)
{
}