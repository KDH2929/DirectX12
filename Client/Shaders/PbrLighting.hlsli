#include "Common.hlsli"

static const float PI = 3.14159265;
static const float3 Fdielectric = float3(0.04, 0.04, 0.04);

Texture2D<float4> textureHeap[4] : register(t0, space0);
SamplerState linearSampler : register(s0);

// IBL textures
TextureCube<float4> irradianceMap : register(t4);
TextureCube<float4> prefilteredMap : register(t5);
Texture2D<float2> brdfLut : register(t6);

struct MaterialPBR
{
    float3 baseColor;
    float metallic; // 16

    float specular;
    float roughness; // 24

    float ambientOcclusion;
    float emissiveIntensity; // 32

    float3 emissiveColor;
    float pad0; // 44+4 =48

    float pad1[4]; // +16 = 64바이트
};


cbuffer CB_MVP : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 modelInvTranspose;
};

cbuffer CB_Lighting : register(b1)
{
    float3 cameraWorld;
    float _padL;
    Light lights[MAX_LIGHTS];
};

cbuffer CB_Material : register(b2)
{
    MaterialPBR material;
};

cbuffer CB_Global : register(b3)
{
    float time;
    float3 _padG;
};

float3 SampleAlbedo(float2 uv)
{
    return textureHeap[0].Sample(linearSampler, uv).rgb;
}
float3 SampleNormal(float2 uv)
{
    // 0~1 범위의 값을 -1 ~ 1 범위의 값으로
    return textureHeap[1].Sample(linearSampler, uv).rgb * 2 - 1;
}
float SampleMetallic(float2 uv)
{
    return textureHeap[2].Sample(linearSampler, uv).r;
}
float SampleRoughness(float2 uv)
{
    return textureHeap[3].Sample(linearSampler, uv).r;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1 - F0) * pow(1 - cosTheta, 5);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a2 = roughness * roughness;
    a2 = a2 * a2;
    float NdotH = saturate(dot(N, H));
    float denom = (NdotH * NdotH) * (a2 - 1) + 1;
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1 - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float k = (roughness + 1);
    k = k * k / 8;
    return GeometrySchlickGGX(saturate(dot(N, V)), k) *
           GeometrySchlickGGX(saturate(dot(N, L)), k);
}

float3 CookTorranceBRDF(float3 N, float3 V, float3 L, float3 F0, float roughness, out float NdotL)
{
    float3 H = normalize(V + L);
    NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);
    return (D * G * F) / max(4 * NdotV * NdotL, 1e-4);
}

float3 DiffuseIBL(float3 N, float3 albedo, float3 V, float metallic)
{
    float3 F0 = lerp(Fdielectric, albedo, metallic);
    float3 F = FresnelSchlick(saturate(dot(N, V)), F0);
    float3 kd = (1 - F) * (1 - metallic);
    float3 irradiance = irradianceMap.Sample(linearSampler, N).rgb;
    return irradiance * albedo * kd * (1.0 / PI);
}

float3 SpecularIBL(float3 N, float3 V, float3 F0, float roughness)
{
    float3 R = reflect(-V, N);
    uint width, height, mipCount;
    prefilteredMap.GetDimensions(0, width, height, mipCount);
    float maxMip = mipCount - 1;
    float mipLevel = roughness * maxMip;
    float3 prefilteredColor = prefilteredMap.SampleLevel(linearSampler, R, mipLevel).rgb;
    float2 brdf = brdfLut.Sample(linearSampler, float2(roughness, saturate(dot(N, V)))).rg;
    return prefilteredColor * (F0 * brdf.x + brdf.y);
}

float3 ComputePBR(float3 worldPos, float3 normal, float2 uv)
{
    // 1) View 벡터, 재질 파라미터 추출
    float3 V = normalize(cameraWorld - worldPos);
    float3 albedo = SampleAlbedo(uv) * material.baseColor;
    float metallic = SampleMetallic(uv) * material.metallic;
    float roughness = SampleRoughness(uv) * material.roughness;
    float ao = material.ambientOcclusion;
    float3 F0_base = float3(material.specular, material.specular, material.specular);
    float3 F0 = lerp(F0_base, albedo, metallic);

    // 2) Direct Lighting (Cook-Torrance)
    float3 Lo = float3(0, 0, 0);
    [unroll]
    for (uint i = 0; i < MAX_LIGHTS; ++i)
    {
        Light Ld = lights[i];
        if (all(Ld.strength == 0))
            continue;

        float3 L;
        float attenuation = 1;
        if (Ld.type == 0)
        {
            L = normalize(-Ld.direction);
        }
        else
        {
            float3 d = Ld.position - worldPos;
            float dist = length(d);
            if (dist > Ld.falloffEnd)
                continue;
            L = d / dist;
            attenuation = saturate((Ld.falloffEnd - dist)
                                      / (Ld.falloffEnd - Ld.falloffStart));
        }

        float NdotL;
        float3 specularTerm = CookTorranceBRDF(normal, V, L, F0, roughness, NdotL);
        float3 kS = FresnelSchlick(saturate(dot(normal, V)), F0);
        float3 kD = (1 - kS) * (1 - metallic);
        float3 diffuseTerm = (kD * albedo) / PI;
        float3 radiance = Ld.strength * Ld.color * attenuation;

        Lo += (diffuseTerm + specularTerm) * radiance * NdotL;
    }

    // 3) IBL (간접광) 기여
    float3 diffuseIBL = DiffuseIBL(normal, albedo, V, metallic) * ao;
    float3 specularIBL = SpecularIBL(normal, V, F0, roughness);

    // 4) Emissive
    float3 emissive = material.emissiveColor * material.emissiveIntensity;

    // 5) 최종 합산 (톤매핑·감마는 필요시 추가)
    return Lo + diffuseIBL + specularIBL + emissive;
}