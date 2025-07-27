#include "Common.hlsli"

struct Material
{
    float3 ambient;
    float _pad0;

    float3 diffuse;
    float shininess;

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
    float3 lightDir = normalize(-L.Direction);
    float3 reflectVec = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0f);
    float spec = pow(max(dot(reflectVec, toEye), 0.0f), M.shininess);

    float3 diffuseTerm = diff * L.Strength * L.Color * M.diffuse;
    float3 specularTerm = spec * L.Strength * L.Color * M.specular;

    return diffuseTerm + specularTerm;
}

float3 ComputePointLight(Light L, Material M, float3 posWorld, float3 normal, float3 toEye)
{
    float3 lightVec = L.Position - posWorld;
    float dist = length(lightVec);
    if (dist > L.Quadratic)
        return float3(0, 0, 0);

    lightVec /= dist;

    // °Å¸® °¨¼è: 1/(C + L¡¤d + Q¡¤d©÷)
    float att = 1.0 / (L.Constant + L.Linear * dist + L.Quadratic * dist * dist);

    float diff = max(dot(normal, lightVec), 0.0f);
    float3 reflectVec = reflect(-lightVec, normal);
    float spec = pow(max(dot(reflectVec, toEye), 0.0f), M.shininess);

    float3 diffuseTerm = diff * L.Strength * L.Color * M.diffuse;
    float3 specularTerm = spec * L.Strength * L.Color * M.specular;

    return (diffuseTerm + specularTerm) * att;
}

float3 ComputeSpotLight(Light L, Material M, float3 posWorld, float3 normal, float3 toEye)
{
    float3 lightVec = L.Position - posWorld;
    float dist = length(lightVec);
    if (dist > L.Quadratic)
        return float3(0, 0, 0);

    lightVec /= dist;

    // 1) °Å¸® °¨¼è
    float distAtt = 1.0 / (L.Constant + L.Linear * dist + L.Quadratic * dist * dist);

    // 2) ½ºÆÌ ÄÆ¿ÀÇÁ °¨¼è (soft edge)
    float cosTheta = dot(lightVec, normalize(-L.Direction));
    float innerCos = cos(L.InnerCutoffAngle);
    float outerCos = cos(L.OuterCutoffAngle);
    float spotAtt = saturate((cosTheta - outerCos) / (innerCos - outerCos));

    float diff = max(dot(normal, lightVec), 0.0f);
    float3 reflectVec = reflect(-lightVec, normal);
    float spec = pow(max(dot(reflectVec, toEye), 0.0f), M.shininess);

    float3 diffuseTerm = diff * L.Strength * L.Color * M.diffuse;
    float3 specularTerm = spec * L.Strength * L.Color * M.specular;

    return (diffuseTerm + specularTerm) * distAtt * spotAtt;
}

float3 ComputeLight(Light L, Material M, float3 posWorld, float3 normal, float3 toEye)
{
    if (L.Type == 0)
        return ComputeDirectionalLight(L, M, normal, toEye);
    else if (L.Type == 1)
        return ComputePointLight(L, M, posWorld, normal, toEye);
    else if (L.Type == 2)
        return ComputeSpotLight(L, M, posWorld, normal, toEye);
    else
        return float3(0, 0, 0);
}
