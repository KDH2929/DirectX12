#include "Common.hlsli"

struct Material
{
    float3 ambient;
    float _pad0;

    float3 diffuse;
    float _pad1;

    float3 specular;
    float alpha;

    bool useAlbedoMap;
    bool useNormalMap;
    float2 _pad2;
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
    float padding0;
    Light lights[MAX_LIGHTS];
}


cbuffer CB_Material : register(b2)
{
    Material material;
}


cbuffer CB_Global : register(b3)
{
    float time;
    float3 padding1; // 16-byte Á¤·Ä
}



float3 ComputeDirectionalLight(Light L, Material M, float3 normal, float3 toEye)
{
    float3 lightDir = normalize(-L.direction);
    float3 reflectVec = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0f);
    float spec = pow(max(dot(reflectVec, toEye), 0.0f), L.specPower);

    float3 diffuseTerm = diff * L.strength * L.color * M.diffuse;
    float3 specularTerm = spec * L.strength * L.color * M.specular;

    return diffuseTerm + specularTerm;
}

float3 ComputePointLight(Light L, Material M, float3 posWorld, float3 normal, float3 toEye)
{
    float3 lightVec = L.position - posWorld;
    float dist = length(lightVec);
    if (dist > L.falloffEnd)
        return float3(0.0f, 0.0f, 0.0f);

    lightVec /= dist;
    float att = saturate((L.falloffEnd - dist) / (L.falloffEnd - L.falloffStart));

    float diff = max(dot(normal, lightVec), 0.0f);
    float3 reflectVec = reflect(-lightVec, normal);
    float spec = pow(max(dot(reflectVec, toEye), 0.0f), L.specPower);

    float3 diffuseTerm = diff * L.strength * L.color * M.diffuse;
    float3 specularTerm = spec * L.strength * L.color * M.specular;

    return (diffuseTerm + specularTerm) * att;
}

float3 ComputeSpotLight(Light L, Material M, float3 posWorld, float3 normal, float3 toEye)
{
    float3 lightVec = L.position - posWorld;
    float dist = length(lightVec);
    if (dist > L.falloffEnd)
        return float3(0.0f, 0.0f, 0.0f);

    lightVec /= dist;
    float att = saturate((L.falloffEnd - dist) / (L.falloffEnd - L.falloffStart));

    float diff = max(dot(normal, lightVec), 0.0f);
    float3 reflectVec = reflect(-lightVec, normal);
    float spec = pow(max(dot(reflectVec, toEye), 0.0f), L.specPower);

    float3 diffuseTerm = diff * L.strength * L.color * M.diffuse;
    float3 specularTerm = spec * L.strength * L.color * M.specular;

    return (diffuseTerm + specularTerm) * att;
}

float3 ComputeLight(Light L, Material M, float3 posWorld, float3 normal, float3 toEye)
{
    if (L.type == 0)
        return ComputeDirectionalLight(L, M, normal, toEye);
    else if (L.type == 1)
        return ComputePointLight(L, M, posWorld, normal, toEye);
    else if (L.type == 2)
        return ComputeSpotLight(L, M, posWorld, normal, toEye);
    else
        return float3(0.0f, 0.0f, 0.0f);
}
