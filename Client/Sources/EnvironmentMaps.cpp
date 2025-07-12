#include "EnvironmentMaps.h"
#include "Renderer.h"

bool EnvironmentMaps::Load(
    Renderer* renderer,
    const std::wstring& irradianceMapPath,
    const std::wstring& specularMapPath,
    const std::wstring& brdfLutPath)
{
    auto textureManager = renderer->GetTextureManager();
    irradianceMap = textureManager->LoadCubeMap(irradianceMapPath);
    specularMap = textureManager->LoadCubeMap(specularMapPath, true);
    brdfLutTexture = textureManager->LoadTexture(brdfLutPath);

    return irradianceMap && specularMap && brdfLutTexture;
}

void EnvironmentMaps::Bind(
    ID3D12GraphicsCommandList* commandList,
    UINT rootIndexIrradiance,
    UINT rootIndexSpecular,
    UINT rootIndexBrdfLut) const
{
    // Descriptor Heaps는 바인딩된 상태여야 함
    commandList->SetGraphicsRootDescriptorTable(
        rootIndexIrradiance,
        irradianceMap->GetGpuHandle());
    commandList->SetGraphicsRootDescriptorTable(
        rootIndexSpecular,
        specularMap->GetGpuHandle());
    commandList->SetGraphicsRootDescriptorTable(
        rootIndexBrdfLut,
        brdfLutTexture->GetGpuHandle());
}
