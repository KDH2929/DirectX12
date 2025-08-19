#include "TextureManager.h"
#include "DescriptorHeapManager.h"
#include "Renderer.h"
#include <directx/d3dx12.h>

bool TextureManager::Initialize(Renderer* renderer_, DescriptorHeapManager* descriptorHeapManager_)
{
    if (!renderer_ || !descriptorHeapManager_)
        return false;

    renderer = renderer_;
    descriptorHeapManager = descriptorHeapManager_;
    return true;
}

std::shared_ptr<Texture> TextureManager::LoadTexture(const std::wstring& filePath, bool generateMips)
{
    // 1) 캐시 조회
    if (auto it = textureCache.find(filePath); it != textureCache.end())
        return it->second;

    // 2) Texture 객체 준비
    auto texture = std::make_shared<Texture>();

    // 3) Texture 내부에서 Copy + Sync 까지 수행
    if (!texture->LoadFromFile(renderer, filePath, generateMips))
        throw std::runtime_error("TextureManager::LoadTexture - load failed");

    // 4) SRV 디스크립터 슬롯 확보 & 생성
    DescriptorHandle handle = descriptorHeapManager->Allocate(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->GetResource()->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    renderer->GetDevice()->CreateShaderResourceView(
        texture->GetResource(), &srvDesc, handle.cpuHandle);

    texture->SetDescriptorHandles(handle.cpuHandle, handle.gpuHandle, handle.index);

    // 5) 캐시에 저장
    textureCache[filePath] = texture;
    return texture;
}

std::shared_ptr<Texture> TextureManager::LoadCubeMap(const std::wstring& filePath, bool generateMips)
{
    // 1) 캐시 조회
    if (auto it = textureCache.find(filePath); it != textureCache.end())
        return it->second;

    // 2) Texture 객체 준비
    auto texture = std::make_shared<Texture>();

    // 3) 큐브맵 로드
    if (!texture->LoadCubeMapFromFile(renderer, filePath, generateMips))
        throw std::runtime_error("TextureManager::LoadCubeMap - load failed");

    // 4) SRV 디스크립터 슬롯 확보 & 생성
    DescriptorHandle handle = descriptorHeapManager->Allocate(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->GetResource()->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = texture->GetResource()->GetDesc().MipLevels;

    renderer->GetDevice()->CreateShaderResourceView(
        texture->GetResource(), &srvDesc, handle.cpuHandle);

    texture->SetDescriptorHandles(handle.cpuHandle, handle.gpuHandle, handle.index);

    // 5) 캐시에 저장
    textureCache[filePath] = texture;
    return texture;
}

void TextureManager::Clear()
{
    textureCache.clear();
}