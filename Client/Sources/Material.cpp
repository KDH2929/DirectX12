#include "Material.h"
#include <cstring>
#include <stdexcept>
#include "ConstantBuffers.h"

void Material::WriteToConstantBuffer(void* constantBuffer) const
{
    CB_MaterialPBR cb{};
    cb.baseColor = parameters.baseColor;
    cb.metallic = parameters.metallic;
    cb.specular = parameters.specular;
    cb.roughness = parameters.roughness;
    cb.ambientOcclusion = parameters.ambientOcclusion;
    cb.emissiveColor = parameters.emissiveColor;
    cb.emissiveIntensity = parameters.emissiveIntensity;

    uint32_t flags = 0;
    if (albedoTexture)    flags |= USE_ALBEDO_MAP;
    if (normalTexture)    flags |= USE_NORMAL_MAP;
    if (metallicTexture)  flags |= USE_METALLIC_MAP;
    if (roughnessTexture) flags |= USE_ROUGHNESS_MAP;
    cb.flags = flags;

    std::memcpy(constantBuffer, &cb, sizeof(cb));
}