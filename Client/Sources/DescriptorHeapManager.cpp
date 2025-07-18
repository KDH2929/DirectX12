#include "DescriptorHeapManager.h"

using HeapType = D3D12_DESCRIPTOR_HEAP_TYPE;

bool DescriptorHeapManager::Initialize(ID3D12Device* device,
    UINT cbvSrvUavCount,
    UINT samplerCount,
    UINT rtvCount,
    UINT dsvCount,
    UINT backBufferCount)
{
    sliceCount = backBufferCount;

    if (!CreateHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cbvSrvUavCount, true)) {
        return false;
    }

    if (!CreateHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, samplerCount, true)) {
        return false;
    }

    if (!CreateHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtvCount, false)) {
        return false;
    }

    if (!CreateHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_DSV, dsvCount, false)) {
        return false;
    }

    sliceSize = cbvSrvUavCount / sliceCount;
    return true;
}

bool DescriptorHeapManager::InitializeImGuiHeaps(
    ID3D12Device* device,
    UINT imguiSrvCount,
    UINT imguiSamplerCount)
{
    // 1) SRV/CBV/UAV 힙 생성
    {
        D3D12_DESCRIPTOR_HEAP_DESC descSrv{};
        descSrv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        descSrv.NumDescriptors = imguiSrvCount;
        descSrv.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        descSrv.NodeMask = 0;
        HRESULT hr = device->CreateDescriptorHeap(&descSrv, IID_PPV_ARGS(&imguiSrvHeap));
        if (FAILED(hr))
            return false;
    }

    // 2) Sampler 힙 생성
    {
        D3D12_DESCRIPTOR_HEAP_DESC descSampler{};
        descSampler.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        descSampler.NumDescriptors = imguiSamplerCount;
        descSampler.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        descSampler.NodeMask = 0;
        HRESULT hr = device->CreateDescriptorHeap(&descSampler, IID_PPV_ARGS(&imguiSamplerHeap));
        if (FAILED(hr))
            return false;
    }

    return true;
}

void DescriptorHeapManager::BeginFrame(UINT backBufferIndex)
{
    auto& cbvHeap = heaps[HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
    cbvHeap.cursor = sliceSize * (backBufferIndex % sliceCount);
}

ID3D12DescriptorHeap* DescriptorHeapManager::GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    return heaps[static_cast<size_t>(type)].heap.Get();
}

UINT DescriptorHeapManager::GetStride(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    return heaps[static_cast<size_t>(type)].stride;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::CreateWrapSampler(ID3D12Device* device)
{
    auto handle = Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    D3D12_SAMPLER_DESC desc{};
    desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc.MinLOD = 0;
    desc.MaxLOD = D3D12_FLOAT32_MAX;
    desc.MipLODBias = 0;
    desc.MaxAnisotropy = 0;
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    device->CreateSampler(&desc, handle.cpu);
    wrapSamplerHandle = handle.gpu;
    return wrapSamplerHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::CreateClampSampler(ID3D12Device* device)
{
    auto handle = Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    D3D12_SAMPLER_DESC desc{};
    desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    desc.MinLOD = 0;
    desc.MaxLOD = D3D12_FLOAT32_MAX;
    desc.MipLODBias = 0;
    desc.MaxAnisotropy = 0;
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    device->CreateSampler(&desc, handle.cpu);
    clampSamplerHandle = handle.gpu;
    return clampSamplerHandle;
}

bool DescriptorHeapManager::CreateRenderTargetView(ID3D12Device* device, ID3D12Resource* resource, UINT heapIndex)
{
    // Allocate 함수를 호출해도됨
    auto& rtvHeap = heaps[static_cast<size_t>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)];
    if (heapIndex >= rtvHeap.capacity) return false;
    D3D12_CPU_DESCRIPTOR_HANDLE handle{
        rtvHeap.cpuBase.ptr + SIZE_T(heapIndex) * rtvHeap.stride
    };

    device->CreateRenderTargetView(resource, nullptr, handle);
    return true;
}

bool DescriptorHeapManager::CreateDepthStencilView(ID3D12Device* device, ID3D12Resource* resource, UINT heapIndex)
{
    // Allocate 함수를 호출해도됨
    auto& dsvHeap = heaps[static_cast<size_t>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)];
    if (heapIndex >= dsvHeap.capacity) return false;
    D3D12_CPU_DESCRIPTOR_HANDLE handle{
        dsvHeap.cpuBase.ptr + SIZE_T(heapIndex) * dsvHeap.stride
    };

    device->CreateDepthStencilView(resource, nullptr, handle);
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetRtvHandle(UINT heapIndex) const
{
    const auto& rtvHeap = heaps[static_cast<size_t>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)];
    return { rtvHeap.cpuBase.ptr + SIZE_T(heapIndex) * rtvHeap.stride };
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetDsvHandle(UINT heapIndex) const
{
    const auto& dsvHeap = heaps[static_cast<size_t>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)];
    return { dsvHeap.cpuBase.ptr + SIZE_T(heapIndex) * dsvHeap.stride };
}

bool DescriptorHeapManager::CreateHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, bool shaderVisible)
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
