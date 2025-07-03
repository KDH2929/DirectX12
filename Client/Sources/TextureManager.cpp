#include "TextureManager.h"
#include "DescriptorHeapManager.h"
#include "Renderer.h"
#include <directx/d3dx12.h>

void TextureManager::Initialize(Renderer* renderer_,
    DescriptorHeapManager* descriptorHeapManager_)
{
    renderer = renderer_;
    descriptorHeapManager = descriptorHeapManager_;
}

std::shared_ptr<Texture> TextureManager::LoadTexture(const std::wstring& filePath)
{
    // 1) ĳ�� ��ȸ
    if (auto it = textureCache.find(filePath); it != textureCache.end())
        return it->second;

    // 2) �ؽ�ó ��ü �غ�
    auto texture = std::make_shared<Texture>();

    ID3D12Device* device = renderer->GetDevice();
    ID3D12GraphicsCommandList* uploadCommandList = renderer->GetUploadCommandList();
    ID3D12CommandAllocator* uploadCommandAlloc = renderer->GetUploadCommandAllocator();
    ID3D12CommandQueue* uploadQueue = renderer->GetUploadQueue();
    ID3D12Fence* uploadFence = renderer->GetUploadFence();

    // ����Ʈ�� �ʱ� ���·�
    uploadCommandList->Close();
    uploadCommandAlloc->Reset();
    uploadCommandList->Reset(uploadCommandAlloc, nullptr);

    if (!texture->LoadFromFile(device, uploadCommandList, filePath))
        throw std::runtime_error("TextureManager::LoadTexture - load failed");

    uploadCommandList->Close();                         // ��� ����

    // 3) Copy ť�� ����
    ID3D12CommandList* submitList[] = { uploadCommandList };
    uploadQueue->ExecuteCommandLists(1, submitList);

    // 4) Signal & Wait (���ε� �Ϸ� ����)
    const UINT64 fenceValue = renderer->IncrementUploadFenceValue();
    uploadQueue->Signal(uploadFence, fenceValue);

    if (uploadFence->GetCompletedValue() < fenceValue)
    {
        HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        uploadFence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);
    }

    // 5) SRV ��ũ���� ���� Ȯ�� & ����
    DescriptorHandle srvHandle = descriptorHeapManager->Allocate(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->GetResource()->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;   // �ʿ� �� MipLevels ����

    device->CreateShaderResourceView(
        texture->GetResource(), &srvDesc, srvHandle.cpu);

    texture->SetDescriptorHandles(srvHandle.cpu, srvHandle.gpu);

    // 6) ĳ�ÿ� ��� �� ��ȯ
    textureCache[filePath] = texture;
    return texture;
}


void TextureManager::Clear()
{
    textureCache.clear();
}
