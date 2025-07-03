#include "DescriptorHeapManager.h"

using HeapType = D3D12_DESCRIPTOR_HEAP_TYPE;

bool DescriptorHeapManager::CreateHeap(ID3D12Device* device,
    HeapType type,
    UINT count,
    bool shaderVisible)
{
    if (count == 0) return true; // allow zero-sized heaps for flexibility

    auto& entry = heaps[static_cast<size_t>(type)];

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = type;
    desc.NumDescriptors = count;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&entry.heap))))
        return false;

    entry.shaderVisible = shaderVisible;
    entry.capacity = count;
    entry.stride = device->GetDescriptorHandleIncrementSize(type);
    entry.cpuBase = entry.heap->GetCPUDescriptorHandleForHeapStart();
    entry.gpuBase = entry.heap->GetGPUDescriptorHandleForHeapStart();
    entry.cursor = 0;
    return true;
}

bool DescriptorHeapManager::Initialize(ID3D12Device* device,
    UINT cbvSrvUavCount,
    UINT samplerCount,
    UINT rtvCount,
    UINT dsvCount,
    UINT backBufferCount)
{
    sliceCount = backBufferCount;

    if (!CreateHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        cbvSrvUavCount, true))  return false;
    if (!CreateHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        samplerCount, true))    return false;
    if (!CreateHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        rtvCount, false))       return false;
    if (!CreateHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        dsvCount, false))       return false;

    sliceSize = cbvSrvUavCount / sliceCount;
    return true;
}

void DescriptorHeapManager::BeginFrame(UINT frameIndex)
{
    auto& cbvHeap = heaps[HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
    cbvHeap.cursor = sliceSize * (frameIndex % sliceCount);
}

ID3D12DescriptorHeap* DescriptorHeapManager::GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    return heaps[static_cast<size_t>(type)].heap.Get();
}

UINT DescriptorHeapManager::GetStride(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    return heaps[static_cast<size_t>(type)].stride;
}


DescriptorHandle DescriptorHeapManager::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT descriptorCount)
{
    HeapInfo& selectedHeap = heaps[static_cast<size_t>(heapType)];

    if (selectedHeap.cursor + descriptorCount > selectedHeap.capacity)
        throw std::runtime_error("DescriptorHeapManager: heap exhausted");

    DescriptorHandle handle;

    handle.cpu.ptr = selectedHeap.cpuBase.ptr + static_cast<SIZE_T>(selectedHeap.cursor) * selectedHeap.stride;

    if (selectedHeap.shaderVisible)
    {
        handle.gpu.ptr = selectedHeap.gpuBase.ptr + static_cast<UINT64>(selectedHeap.cursor) * selectedHeap.stride;
    }
    else
    {
        handle.gpu.ptr = 0;
    }

    handle.index = selectedHeap.cursor;
    selectedHeap.cursor += descriptorCount;

    return handle;
}
