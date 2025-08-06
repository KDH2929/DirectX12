#include "Common.hlsli"

// pbr 참고코드 : https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/1.1.lighting/1.1.pbr.fs

static const float PI = 3.14159265;
static const float3 F_DIELECTRIC = float3(0.04, 0.04, 0.04);

// 0=Albedo, 1=Normal, 2=Metallic, 3=Roughness
// 추후 하나의 텍스쳐의 R, G, B, A 채널로 값을 전달하도록 변경해야함
Texture2D<float4> textureHeap[4] : register(t0, space0);

// Sampler
SamplerState linearWrapSampler : register(s0);
SamplerState linearClampSampler : register(s1);

// IBL Texture
TextureCube<float4> irradianceMap : register(t4, space0);
TextureCube<float4> prefilteredMap : register(t5, space0);
Texture2D<float2> brdfLut : register(t6, space0);


// Shadow depth maps
Texture2D<float> shadowDepthMap[MAX_SHADOW_DSV_COUNT] : register(t7, space0);
SamplerComparisonState shadowMapSampler : register(s2);    // 깊이비교용 샘플러


struct MaterialPBR
{
    float3 baseColor;
    float metallic;

    float specular;
    float roughness;
    float ambientOcclusion;
    float emissiveIntensity;

    float3 emissiveColor;
    uint flags;

    float4 padding;
};

cbuffer CB_MVP : register(b0)
{
    float4x4 model, view, projection, modelInvTranspose;
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

cbuffer CB_ShadowMapViewProj : register(b4)
{
    float4x4 ShadowMapViewProj[MAX_SHADOW_DSV_COUNT];
};

// Map flags
static const uint USE_ALBEDO_MAP = (1 << 0);
static const uint USE_NORMAL_MAP = (1 << 1);
static const uint USE_METALLIC_MAP = (1 << 2);
static const uint USE_ROUGHNESS_MAP = (1 << 3);

bool HasMap(uint flag)
{
    return (material.flags & flag) != 0;
}

float3 SampleAlbedo(float2 uv)
{
    if (HasMap(USE_ALBEDO_MAP))
    {
        return textureHeap[0].Sample(linearWrapSampler, uv).rgb * material.baseColor;
    }
    return material.baseColor;
}

float3 SampleNormal(float2 uv)
{
    if (HasMap(USE_NORMAL_MAP))
    {
        float3 n = textureHeap[1].Sample(linearWrapSampler, uv).rgb * 2 - 1;
        return normalize(n);
    }
    return float3(0, 0, 1);
}

float SampleMetallic(float2 uv)
{
    float m = material.metallic;
    if (HasMap(USE_METALLIC_MAP))
    {
        // Flight Asset 의 alpha 채널에 Roughness 텍스쳐를 넣어놓았음. 
        m *= textureHeap[2].Sample(linearWrapSampler, uv).a;
    }
    return saturate(m);
}

float SampleRoughness(float2 uv)
{
    float r = material.roughness;
    if (HasMap(USE_ROUGHNESS_MAP))
    {
        r *= textureHeap[3].Sample(linearWrapSampler, uv).r;
    }
    return saturate(r);
}

float SampleAO()
{
    return saturate(material.ambientOcclusion);
}

// Fresnel-Schlick
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1 - F0) * pow(1 - cosTheta, 5);
}

// GGX NDF
float DistributionGGX(float3 N, float3 H, float rough)
{
    float a2 = rough * rough;
    a2 = a2 * a2;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH) * (a2 - 1) + 1;
    return a2 / (PI * denom * denom);
}

// Schlick-GGX geometry term
float GeometrySchlickGGX(float NdotV, float rough)
{
    float r = rough + 1;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1 - k) + k);
}

// Smith’s method
float GeometrySmith(float3 N, float3 V, float3 L, float rough)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, rough) * GeometrySchlickGGX(NdotL, rough);
}

// Cook-Torrance specular
float3 BRDF_Specular(float3 N, float3 V, float3 L, float3 F0, float roughness)
{
    float3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = saturate(dot(H, V));

    // Microfacet terms
    float D = DistributionGGX(N, H, roughness); // NDF
    float G = GeometrySmith(N, V, L, roughness); // Geometry
    float3 F = FresnelSchlick(NdotH, F0); // Fresnel

    // Cook-Torrance BRDF
    float3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001; // +ε to avoid divide-by-zero
    return numerator / denominator;
}

float3 BRDF_Diffuse(float3 albedo, float3 N, float3 V, float3 L, float3 F0, float metallic)
{
    float NdotV = max(dot(N, V), 0.0);
    float3 kS = FresnelSchlick(NdotV, F0);
    float3 kD = (1 - kS) * (1 - metallic);
    return kD * albedo / PI;
}


// IBL ambient terms
float3 IBL_Diffuse(float3 albedo, float3 N)
{
    float3 irradiance = irradianceMap.SampleLevel(linearWrapSampler, N, 0).rgb;
    return irradiance * albedo;
}

float3 IBL_Specular(float3 F0, float3 N, float3 V, float roughness)
{
    uint w, h, mips;
    prefilteredMap.GetDimensions(0, w, h, mips);
    float3 R = reflect(-V, N);
    float lod = roughness * (mips - 1);
    float3 prefiltered = prefilteredMap.SampleLevel(linearClampSampler, R, lod).rgb;

    float NdotV = max(dot(N, V), 0.0);
    float2 brdfSample = brdfLut.Sample(linearClampSampler, float2(NdotV, roughness)).rg;

    return prefiltered * (F0 * brdfSample.x + brdfSample.y);
}

float3 ComputePBR(float3 worldPos, float3 worldNormal, float2 uv)
{
    float3 N = normalize(worldNormal);
    float3 V = normalize(cameraWorld- worldPos);
    
    float3 albedo = SampleAlbedo(uv);
    float metal = SampleMetallic(uv);
    float rough = SampleRoughness(uv);
    float ao = SampleAO();

    float3 F0 = lerp(F_DIELECTRIC, albedo, metal);
    float3 accumulatedLight = float3(0, 0, 0);

    [unroll]
    for (uint i = 0; i < MAX_LIGHTS; ++i)
    {
        Light currentLight = lights[i];
        if (all(currentLight.Color * currentLight.Strength == 0))
            continue;

        float3 L;
        float attenuation = 1.0;

        if (currentLight.Type == 0)  // Directional
        {
            L = normalize(-currentLight.Direction);
        }
        else
        {
            float3 toLight = currentLight.Position - worldPos;
            float dist = length(toLight);
            L = toLight / dist;

            // 거리 감쇠
            attenuation = 1.0 / (currentLight.Constant + currentLight.Linear * dist + currentLight.Quadratic * dist * dist);

            if (currentLight.Type == 2)  // Spot
            {
                float cosTheta = dot(L, normalize(-currentLight.Direction));
                float innerCos = cos(currentLight.InnerCutoffAngle);
                float outerCos = cos(currentLight.OuterCutoffAngle);
                float spotAttenuation = saturate((cosTheta - outerCos) / (innerCos - outerCos));
                attenuation *= spotAttenuation;
            }
        }

        float NdotL = max(dot(N, L), 0.0);
        float3 radiance = currentLight.Color * currentLight.Strength * attenuation;

        float3 specular = BRDF_Specular(N, V, L, F0, rough);
        float3 diffuse = BRDF_Diffuse(albedo, N, V, L, F0, metal);
    
        accumulatedLight += (diffuse + specular) * radiance * NdotL;
    }

    float3 ambient = IBL_Diffuse(albedo, N) * ao + IBL_Specular(F0, N, V, rough);
    return accumulatedLight + ambient;
}



// 3×3 PCF 샘플링
float SampleShadowMapPCF(int idx, float4 posLightSpace)
{
    float3 ndc = posLightSpace.xyz / posLightSpace.w;
    float2 uv = float2(ndc.x, -ndc.y) * 0.5f + 0.5f;      // y는 음수를 붙여서
    float depth = ndc.z;

    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
    {
        return 1.0f;
    }

    // 절두체 검사 (DirectX NDC 기준)
    if (abs(ndc.x) > 1.0f || abs(ndc.y) > 1.0f || ndc.z < 0.0f || ndc.z > 1.0f)
    {
        return 1.0f; // 절두체 밖 → 그림자 없음 처리
    }
    
    float sum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float bias = 1.5f * INV_SHADOW_MAP_HEIGHT;
            float2 offset = float2(x * INV_SHADOW_MAP_WIDTH, y * INV_SHADOW_MAP_HEIGHT);
            sum += shadowDepthMap[idx].SampleCmp(shadowMapSampler, uv + offset, depth - bias);
        }
    }
    
    return sum / 9.0f;
}


float SampleShadowMapHard(int idx, float4 posLightSpace)
{
    
    float3 ndc = posLightSpace.xyz / posLightSpace.w;
    float2 uv = float2(ndc.x, -ndc.y) * 0.5f + 0.5f; // y는 음수를 붙여서
    float depth = ndc.z;

    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
    {
        return 1.0f;
    }

    // 절두체 검사 (DirectX NDC 기준)
    if (abs(ndc.x) > 1.0f || abs(ndc.y) > 1.0f || ndc.z < 0.0f || ndc.z > 1.0f)
    {
        return 1.0f; // 절두체 밖 → 그림자 없음 처리
    }
    
    
    float bias = 1.5f * INV_SHADOW_MAP_HEIGHT;
    return shadowDepthMap[idx].SampleCmp(shadowMapSampler, uv, depth - bias);
}

float3 SampleShadowMapDebug(int idx, float4 posLightSpace)
{

    float3 ndc = posLightSpace.xyz / posLightSpace.w;
    float2 uv = float2(ndc.x, -ndc.y) * 0.5f + 0.5f;
    float depth = ndc.z;
    

    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f ||
        abs(ndc.x) > 1.0f || abs(ndc.y) > 1.0f || depth < 0.0f || depth > 1.0f)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    

    float bias = 1.5f * INV_SHADOW_MAP_HEIGHT;
    
    // raw map depth (R 채널)
    float mapDepth = shadowDepthMap[idx].Sample(linearClampSampler, uv).r;
    float refDepth = depth - bias;
    float cmpMask = (mapDepth <= refDepth) ? 0.0f : 1.0f;
    
    // 결과를 RGB로 한 번에 리턴하여 디버깅
    return float3(mapDepth, refDepth, cmpMask);
}


float3 ComputePBRWithShadow(float3 worldPos, float3 worldNormal, float2 uv)
{
    float3 N = normalize(worldNormal);
    float3 V = normalize(cameraWorld- worldPos);
    
    float3 albedo = SampleAlbedo(uv);
    float metal = SampleMetallic(uv);
    float rough = SampleRoughness(uv);
    float ao = SampleAO();

    float3 F0 = lerp(F_DIELECTRIC, albedo, metal);
    float3 accumulatedLight = float3(0, 0, 0);
    
    
    int shadowMapIdx = 0;

    [unroll]
    for (uint i = 0; i < MAX_LIGHTS; ++i)
    {
        Light currentLight = lights[i];
        
        int faceCount = (currentLight.Type == 1) ? 6 : 1;     // Point=6 (큐브맵), 나머지=1

        if (all(currentLight.Color * currentLight.Strength == 0))
        {
            shadowMapIdx += faceCount;
            continue;
        }
        
        if (currentLight.ShadowCastingEnabled == 0)
        {
            shadowMapIdx += faceCount;
            continue;
        }

        // 기존 PBR 루프에서처럼 Radiance 계산
        float3 L;       // L = lightDirection
        float attenuation = 1.0f;

        if (currentLight.Type == 0)  // Directional
        {
            L = normalize(-currentLight.Direction);
        }
        else
        {
            float3 toLight = currentLight.Position - worldPos;
            float dist = length(toLight);
            L = toLight / dist;

            // 거리 감쇠
            attenuation = 1.0f / (currentLight.Constant + currentLight.Linear * dist + currentLight.Quadratic * dist * dist);

            // Spot 감쇠
            if (currentLight.Type == 2)
            {
                float cosTheta = dot(L, normalize(-currentLight.Direction));
                float innerCos = cos(currentLight.InnerCutoffAngle);
                float outerCos = cos(currentLight.OuterCutoffAngle);
                float spotAttenuation = saturate((cosTheta - outerCos) / (innerCos - outerCos));
                attenuation *= spotAttenuation;
            }
        }

        float NdotL = max(dot(N, L), 0.0f);
        float3 radiance = currentLight.Color * currentLight.Strength * attenuation;

        float3 specular = BRDF_Specular(N, V, L, F0, rough);
        float3 diffuse = BRDF_Diffuse(albedo, N, V, L, F0, metal);

        float3 lightContribute = (diffuse + specular) * radiance * NdotL;
        
        float lightFactor = 0.0f; // 0 = 그림자,  1 = lit

        [unroll]
        for (int f = 0; f < faceCount; ++f)
        {
            float4 posLightSpace = mul(float4(worldPos, 1), ShadowMapViewProj[shadowMapIdx + f]);
            lightFactor += SampleShadowMapPCF(shadowMapIdx + f, posLightSpace);
        }
        
        shadowMapIdx += faceCount;
        
        // 6면에 대한 그림자 영향을 평균
        lightFactor /= faceCount;
        
        float3 shadowColor = float3(0.0f, 0.0f, 0.0f);
        
        // 감마 = 3.0 으로 조정
        // 그림자 색상을 더 잘 보이게 하기 위함
        lightFactor = pow(lightFactor, 2.0f);
        
        // 빛 기여도에 따라 litColor와 shadowColor 사이를 보간
        float3 shadedContribution = lerp(shadowColor, lightContribute, lightFactor);

        accumulatedLight += shadedContribution;
    }

    // Ambient(IBL) 은 그림자의 영향을 받지 않음
    float3 ambient = IBL_Diffuse(albedo, N) * ao
                   + IBL_Specular(F0, N, V, rough);

    return accumulatedLight + ambient;
}


// 디버깅용
float3 ComputeShadowMask(float3 worldPos)
{
    float4 posLS = mul(float4(worldPos, 1), ShadowMapViewProj[0]);
    
    float3 mask = SampleShadowMapDebug(0, posLS);
    
    return mask;
}