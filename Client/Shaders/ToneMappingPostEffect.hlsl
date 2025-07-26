cbuffer CB_ToneMapping : register(b0)
{
    float Exposure;     // Brightness control (e.g., 0.1 ~ 5.0)
    float Gamma;        // Gamma correction (e.g., 2.2)
    float2 _padding;    // Padding to 16-byte boundary
};

Texture2D<float4> SceneColor  : register(t0);
SamplerState   SceneSampler : register(s0);

// Full-screen quad input/output
struct VSInput
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD0;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.Position = float4(input.Position, 1.0);
    output.UV = input.UV;
    return output;
}

// Reinhard tone-mapping function
float3 Tonemap_Reinhard(float3 x)
{
    return x / (x + 1.0);
}

float4 PSMain(PSInput input) : SV_Target
{

    float3 hdr = SceneColor.Sample(SceneSampler, input.UV).rgb;
    hdr *= Exposure;

    float3 mapped = Tonemap_Reinhard(hdr);

    // Gamma correction
    mapped = pow(mapped, 1.0 / Gamma);

    return float4(mapped, 1.0);
}
