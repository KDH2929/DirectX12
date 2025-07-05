#include "Material.h"
#include <cstring>
#include <stdexcept>
#include "ConstantBuffers.h"

void Material::WriteToGpu(void* destination) const
{
    if (!destination)
        throw std::runtime_error("Material::WriteToGpu - destination is null");

    CB_MaterialPBR cb{};
    cb.baseColor = parameters.baseColor;
    cb.metallic = parameters.metallic;
    cb.specular = parameters.specular;
    cb.roughness = parameters.roughness;
    cb.ambientOcclusion = parameters.ambientOcclusion;
    cb.emissiveColor = parameters.emissiveColor;
    cb.emissiveIntensity = parameters.emissiveIntensity;

    std::memcpy(destination, &cb, sizeof(cb));
}
