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

DescriptorHandle DescriptorHeapManager::Allocate(HeapType type, UINT count)
{
    auto& e = heaps[static_cast<size_t>(type)];
    if (e.cursor + count > e.capacity)
        throw std::runtime_error("DescriptorHeapManager: heap exhausted");

    DescriptorHandle h;
    h.cpu.ptr = e.cpuBase.ptr + static_cast<SIZE_T>(e.cursor) * e.stride;
    h.gpu.ptr = e.shaderVisible ?
        e.gpuBase.ptr + static_cast<UINT64>(e.cursor) * e.stride : 0;
    h.index = e.cursor;

    e.cursor += count;
    return h;
}
