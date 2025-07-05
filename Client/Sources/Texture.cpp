#include "Texture.h"
#include "Renderer.h"
#include <DirectXTex.h>
#include <directx/d3dx12.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// ������ �ε��Ͽ� GPU �ؽ�ó ���� �� COMMON ���·� ��ȯ
// (Copy ��� Close �� Execute �� Fence Wait�� ���ο��� ó��)

bool Texture::LoadFromFile(Renderer* renderer,
    const std::wstring& filePath)
{
    name = filePath;

    // ���������� Copy ���� Ŀ�ǵ� ����Ʈ/�Ҵ���/ť/�潺 ���� ��������
    ID3D12Device* device = renderer->GetDevice();
    ID3D12GraphicsCommandList* copyCommandList = renderer->GetCopyCommandList();
    ID3D12CommandAllocator* copyCommandAllocator = renderer->GetCopyCommandAllocator();
    ID3D12CommandQueue* copyQueue = renderer->GetCopyQueue();
    ID3D12Fence* copyFence = renderer->GetCopyFence();
    UINT64& copyFenceValue = renderer->GetCopyFenceValue();

    // 1) �̹��� ���� �ε�
    ScratchImage image;
    if (FAILED(LoadFromWICFile(filePath.c_str(),
        WIC_FLAGS_FORCE_RGB, nullptr, image)))
    {
        return false;
    }
    const Image* src = image.GetImage(0, 0, 0);

    // 2) DEFAULT-heap �ؽ�ó ���� (�ʱ� ���� = COMMON)
    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC textureDesc =
        CD3DX12_RESOURCE_DESC::Tex2D(src->format, src->width, src->height, 1, 1);

    if (FAILED(device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&texture))))
    {
        return false;
    }

    // 3) UPLOAD-heap �߰� ���� ����
    UINT64 requiredSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc =
        CD3DX12_RESOURCE_DESC::Buffer(requiredSize);

    ComPtr<ID3D12Resource> stagingBuffer;
    if (FAILED(device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&stagingBuffer))))
    {
        return false;
    }

    // 4) CPU �� UPLOAD ���� �޸� ����
    void* mappedData = nullptr;
    stagingBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, src->pixels, src->slicePitch);
    stagingBuffer->Unmap(0, nullptr);

    // 5) Copy Ŀ�ǵ� ����Ʈ �غ�
    copyCommandAllocator->Reset();
    copyCommandList->Reset(copyCommandAllocator, nullptr);

    // COMMON �� COPY_DEST ��ȯ
    {
        CD3DX12_RESOURCE_BARRIER barrierToCopy =
            CD3DX12_RESOURCE_BARRIER::Transition(
                texture.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST);
        copyCommandList->ResourceBarrier(1, &barrierToCopy);
    }

    // ���긮�ҽ� ����
    D3D12_SUBRESOURCE_DATA subresourceData = {
        src->pixels,
        src->rowPitch,
        src->slicePitch
    };
    UpdateSubresources(
        copyCommandList,
        texture.Get(),
        stagingBuffer.Get(),
        0, 0, 1,
        &subresourceData);

    // COPY_DEST �� COMMON ��ȯ
    {
        CD3DX12_RESOURCE_BARRIER barrierToCommon =
            CD3DX12_RESOURCE_BARRIER::Transition(
                texture.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_COMMON);
        copyCommandList->ResourceBarrier(1, &barrierToCommon);
    }

    copyCommandList->Close();

    // 6) Copy ť�� ���� �� ����ȭ
    ID3D12CommandList* lists[] = { copyCommandList };
    copyQueue->ExecuteCommandLists(1, lists);

    ++copyFenceValue;
    copyQueue->Signal(copyFence, copyFenceValue);

    if (copyFence->GetCompletedValue() < copyFenceValue)
    {
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        copyFence->SetEventOnCompletion(copyFenceValue, eventHandle);
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    // stagingBuffer�� �Լ� ���� �� �ڵ� ������
    return true;
}

ID3D12Resource* Texture::GetResource() const { return texture.Get(); }
D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetGpuHandle() const { return gpuHandle; }
D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetCpuHandle() const { return cpuHandle; }
UINT Texture::GetDescriptorIndex() const { return descriptorIndex; }
const std::wstring& Texture::GetName() const { return name; }

void Texture::SetDescriptorHandles(
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle_,
    UINT index_)
{
    cpuHandle = cpuHandle_;
    gpuHandle = gpuHandle_;
    descriptorIndex = index_;
}
