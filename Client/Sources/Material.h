#pragma once

#include <memory>
#include <DirectXMath.h>
#include <cstdint>
#include "Texture.h"


struct MaterialPbrParameters
{
    DirectX::XMFLOAT3 baseColor{ 1.f, 1.f, 1.f };
    float             metallic = 0.5f;
    float             specular = 0.5f;
    float             roughness = 0.f;
    float             ambientOcclusion = 1.f;
    DirectX::XMFLOAT3 emissiveColor = { 0.f, 0.f, 0.f };
    float             emissiveIntensity = 0.f;
};

struct MaterialPbrTextures
{
    std::shared_ptr<Texture> albedoTexture = nullptr;
    std::shared_ptr<Texture> normalTexture = nullptr;
    std::shared_ptr<Texture> metallicTexture = nullptr;
    std::shared_ptr<Texture> roughnessTexture = nullptr;
    std::shared_ptr<Texture> ambientOcclusionTexture = nullptr;
    std::shared_ptr<Texture> emissiveTexture = nullptr;
};

class Material
{
public:
    MaterialPbrParameters parameters;

    void SetAlbedoTexture(std::shared_ptr<Texture> t) { albedoTexture = std::move(t); }
    void SetNormalTexture(std::shared_ptr<Texture> t) { normalTexture = std::move(t); }
    void SetMetallicTexture(std::shared_ptr<Texture> t) { metallicTexture = std::move(t); }
    void SetRoughnessTexture(std::shared_ptr<Texture> t) { roughnessTexture = std::move(t); }
    void SetAmbientOcclusionTexture(std::shared_ptr<Texture> t) { ambientOcclusionTexture = std::move(t); }
    void SetEmissiveTexture(std::shared_ptr<Texture> t) { emissiveTexture = std::move(t); }

    void SetAllTextures(const MaterialPbrTextures& textures)
    {
        albedoTexture = textures.albedoTexture;
        normalTexture = textures.normalTexture;
        metallicTexture = textures.metallicTexture;
        roughnessTexture = textures.roughnessTexture;
        ambientOcclusionTexture = textures.ambientOcclusionTexture;
        emissiveTexture = textures.emissiveTexture;
    }

    // Texture getters
    std::shared_ptr<Texture> GetAlbedoTexture() const { return albedoTexture; }
    std::shared_ptr<Texture> GetNormalTexture() const { return normalTexture; }
    std::shared_ptr<Texture> GetMetallicTexture() const { return metallicTexture; }
    std::shared_ptr<Texture> GetRoughnessTexture() const { return roughnessTexture; }
    std::shared_ptr<Texture> GetAmbientOcclusionTexture() const { return ambientOcclusionTexture; }
    std::shared_ptr<Texture> GetEmissiveTexture() const { return emissiveTexture; }

    // GPU에서 사용할 CB_MaterialPBR 채우는 함수
    void WriteToConstantBuffer(void* constantBuffer) const;

private:
    std::shared_ptr<Texture> albedoTexture;
    std::shared_ptr<Texture> normalTexture;
    std::shared_ptr<Texture> metallicTexture;
    std::shared_ptr<Texture> roughnessTexture;
    std::shared_ptr<Texture> ambientOcclusionTexture;
    std::shared_ptr<Texture> emissiveTexture;
};