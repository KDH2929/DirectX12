#include "Texture.h"
#include <DirectXTex.h>
#include <directx/d3dx12.h>

using namespace DirectX;

bool Texture::LoadFromFile(ID3D12Device* device,
    ID3D12GraphicsCommandList* copyCommandList,
    const std::wstring& filePath)
{
    name = filePath;

    // 1) 이미지 로드
    ScratchImage imageContainer;
    if (FAILED(LoadFromWICFile(filePath.c_str(),
        WIC_FLAGS_FORCE_RGB, nullptr, imageContainer)))
        return false;

    const Image* image = imageContainer.GetImage(0, 0, 0);

    // 2) DEFAULT-heap 텍스처
    CD3DX12_RESOURCE_DESC textureDesc =
        CD3DX12_RESOURCE_DESC::Tex2D(
            image->format, image->width, image->height, 1, 1);

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    if (FAILED(device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(texture.GetAddressOf()))))
        return false;

    // 3) UPLOAD-heap 중간 버퍼
    UINT64 uploadBytes =
        GetRequiredIntermediateSize(texture.Get(), 0, 1);

    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc =
        CD3DX12_RESOURCE_DESC::Buffer(uploadBytes);

    if (FAILED(device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf()))))
        return false;

    // 4) 서브리소스 복사
    D3D12_SUBRESOURCE_DATA subresourceData{};
    subresourceData.pData = image->pixels;
    subresourceData.RowPitch = image->rowPitch;
    subresourceData.SlicePitch = image->slicePitch;

    UpdateSubresources(copyCommandList,
        texture.Get(), uploadBuffer.Get(),
        0, 0, 1, &subresourceData);

    // 5) COPY_DEST → COMMON (Copy 큐에서 허용되는 상태)
    CD3DX12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            texture.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COMMON);
    copyCommandList->ResourceBarrier(1, &barrier);

    return true;                 // Direct 큐에서 COMMON → SRV 상태 전환 예정
}

void Texture::SetDescriptorHandles(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle_)
{
    cpuHandle = cpuHandle_;
    gpuHandle = gpuHandle_;
}

ID3D12Resource* Texture::GetResource() const { return texture.Get(); }
D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetGpuHandle() const { return gpuHandle; }
D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetCpuHandle() const { return cpuHandle; }
const std::wstring& Texture::GetName() const { return name; }
