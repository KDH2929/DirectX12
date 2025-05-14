TextureCube skyboxTex : register(t0);
SamplerState samplerState : register(s0);

struct VSOutput {
    float4 pos : SV_POSITION;
    float3 direction : TEXCOORD;
};

float4 PSMain(VSOutput input) : SV_Target
{
    return skyboxTex.Sample(samplerState, input.direction);
}
