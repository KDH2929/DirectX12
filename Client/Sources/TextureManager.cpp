#include "TextureManager.h"
#include "DescriptorHeapManager.h"
#include "Renderer.h"
#include <directx/d3dx12.h>

void TextureManager::Initialize(Renderer* renderer_, DescriptorHeapManager* descriptorHeapManager_)
{
    renderer = renderer_;
    descriptorHeapManager = descriptorHeapManager_;
}

std::shared_ptr<Texture> TextureManager::LoadTexture(const std::wstring& filePath)
{
    // 1) ĳ�� ��ȸ
    if (auto it = textureCache.find(filePath); it != textureCache.end())
        return it->second;

    // 2) Texture ��ü �غ�
    auto texture = std::make_shared<Texture>();

    // 3) Texture ���ο��� Copy + Sync ���� ����
    if (!texture->LoadFromFile(renderer, filePath))
        throw std::runtime_error("TextureManager::LoadTexture - load failed");

    // 4) SRV ��ũ���� ���� Ȯ�� & ����
    DescriptorHandle handle = descriptorHeapManager->Allocate(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->GetResource()->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    renderer->GetDevice()->CreateShaderResourceView(
        texture->GetResource(), &srvDesc, handle.cpu);

    texture->SetDescriptorHandles(handle.cpu, handle.gpu, handle.index);

    // 5) ĳ�ÿ� ����
    textureCache[filePath] = texture;
    return texture;
}

void TextureManager::Clear()
{
    textureCache.clear();
}
