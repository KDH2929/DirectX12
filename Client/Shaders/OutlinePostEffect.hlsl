cbuffer CB_OutlineOptions : register(b0)
{
    float2 TexelSize;       // 1.0 / Texture dimensions
    float  EdgeThreshold;   // Multiplier to adjust edge visibility
    float  EdgeThickness;
    float3 OutlineColor;
    float  MixFactor;       // Blend factor between original and outline (0=only original, 1=only outline)
};

// Input render target
Texture2D<float4> SceneColor : register(t0);
SamplerState SceneSampler : register(s0);

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


float4 PSMain(PSInput input) : SV_Target
{

    float filterSum = 0.0;
    float2 uv = input.UV;
    // step : uv offset �� ó���ϱ� ���� ��
    float2 step = TexelSize * EdgeThickness;

    // ���ö�þ� ����
    float kernel[3][3] = {
        { 0,  1,  0},
        { 1, -4,  1},
        { 0,  1,  0}
    };

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 offset = float2(x, y) * step;
            float3 color = SceneColor.Sample(SceneSampler, uv + offset).rgb;
            float luminance = dot(color, float3(0.299, 0.587, 0.114));
            filterSum += luminance * kernel[y + 1][x + 1];
        }
    }

    float edgeStrength = abs(filterSum);
    float edgeMask = saturate(edgeStrength * EdgeThreshold);

    // ���� Scene ����
    float3 original = SceneColor.Sample(SceneSampler, uv).rgb;

    // Lerp �� �ܰ�������� Scene ������ ȥ��
    float3 result = lerp(original, OutlineColor, edgeMask * MixFactor);
    return float4(result, 1.0);
}