#include "DescriptorHeapManager.h"

using HeapType = D3D12_DESCRIPTOR_HEAP_TYPE;

bool DescriptorHeapManager::Initialize(ID3D12Device* device,
    UINT cbvSrvUavCount,
    UINT samplerCount,
    UINT rtvCount,
    UINT dsvCount,
    UINT backBufferCount)
{

    if (!CreateDescriptorHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cbvSrvUavCount, true)) {
        return false;
    }

    if (!CreateDescriptorHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, samplerCount, true)) {
        return false;
    }

    if (!CreateDescriptorHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtvCount, false)) {
        return false;
    }

    if (!CreateDescriptorHeap(device, HeapType::D3D12_DESCRIPTOR_HEAP_TYPE_DSV, dsvCount, false)) {
        return false;
    }

    return true;
}

bool DescriptorHeapManager::InitializeImGuiDescriptorHeaps(
    ID3D12Device* device,
    UINT imguiSrvCount,
    UINT imguiSamplerCount)
{
    // 1) SRV/CBV/UAV 赛 积己
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

    // 2) Sampler 赛 积己
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

ID3D12DescriptorHeap* DescriptorHeapManager::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    return descriptorHeaps[static_cast<size_t>(type)].descriptorHeap.Get();
}

UINT DescriptorHeapManager::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    return descriptorHeaps[static_cast<size_t>(type)].descriptorSize;
}


D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::CreateLinearWrapSampler(ID3D12Device* device)
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

    device->CreateSampler(&desc, handle.cpuHandle);
    linearWrapSamplerHandle = handle.gpuHandle;
    return linearWrapSamplerHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::CreateLinearClampSampler(ID3D12Device* device)
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

    device->CreateSampler(&desc, handle.cpuHandle);
    linearClampSamplerHandle = handle.gpuHandle;
    return linearClampSamplerHandle;
}

bool DescriptorHeapManager::CreateRenderTargetView(
    ID3D12Device* device,
    ID3D12Resource* resource,
    UINT descriptorIndex)
{
    auto& rtvInfo = descriptorHeaps[static_cast<size_t>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)];
    if (descriptorIndex >= rtvInfo.maxDescriptors)
        return false;

    D3D12_CPU_DESCRIPTOR_HANDLE handle{
        rtvInfo.cpuStart.ptr + SIZE_T(descriptorIndex) * rtvInfo.descriptorSize
    };

    device->CreateRenderTargetView(resource, nullptr, handle);
    return true;
}

bool DescriptorHeapManager::CreateDepthStencilView(
    ID3D12Device* device,
    ID3D12Resource* resource,
    const D3D12_DEPTH_STENCIL_VIEW_DESC* desc,
    UINT descriptorIndex)
{
    auto& dsvInfo = descriptorHeaps[static_cast<size_t>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)];
    if (descriptorIndex >= dsvInfo.maxDescriptors)
        return false;

    D3D12_CPU_DESCRIPTOR_HANDLE handle{
        dsvInfo.cpuStart.ptr + SIZE_T(descriptorIndex) * dsvInfo.descriptorSize
    };

    device->CreateDepthStencilView(resource, desc, handle);
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetRtvCpuHandle(UINT descriptorIndex) const
{
    const auto& rtvInfo = descriptorHeaps[static_cast<size_t>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)];
    return { rtvInfo.cpuStart.ptr + SIZE_T(descriptorIndex) * rtvInfo.descriptorSize };
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetDsvCpuHandle(UINT descriptorIndex) const
{
    const auto& dsvInfo = descriptorHeaps[static_cast<size_t>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)];
    return { dsvInfo.cpuStart.ptr + SIZE_T(descriptorIndex) * dsvInfo.descriptorSize };
}

DescriptorHandle DescriptorHeapManager::GetRtvHandle(UINT descriptorIndex) const
{
    const auto& rtvInfo = descriptorHeaps[static_cast<size_t>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)];
    DescriptorHandle handle;
    handle.cpuHandle.ptr = rtvInfo.cpuStart.ptr + SIZE_T(descriptorIndex) * rtvInfo.descriptorSize;
    handle.gpuHandle.ptr = 0;
    handle.index = descriptorIndex;
    return handle;
}

DescriptorHandle DescriptorHeapManager::GetDsvHandle(UINT descriptorIndex) const
{
    const auto& dsvInfo = descriptorHeaps[static_cast<size_t>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)];
    DescriptorHandle handle;
    handle.cpuHandle.ptr = dsvInfo.cpuStart.ptr + SIZE_T(descriptorIndex) * dsvInfo.descriptorSize;
    handle.gpuHandle.ptr = 0;
    handle.index = descriptorIndex;
    return handle;
}

bool DescriptorHeapManager::CreateDescriptorHeap(
    ID3D12Device* device,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    UINT descriptorCount,
    bool shaderVisible)
{
    if (descriptorCount == 0)
        return true;

    auto& info = descriptorHeaps[static_cast<size_t>(type)];

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = type;
    desc.NumDescriptors = descriptorCount;
    desc.Flags = shaderVisible
        ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(device->CreateDescriptorHeap(&desc,
        IID_PPV_ARGS(&info.descriptorHeap))))
        return false;

    info.isShaderVisible = shaderVisible;
    info.maxDescriptors = descriptorCount;
    info.descriptorSize =
        device->GetDescriptorHandleIncrementSize(type);
    info.cpuStart = info.descriptorHeap
        ->GetCPUDescriptorHandleForHeapStart();
    info.gpuStart = info.descriptorHeap
        ->GetGPUDescriptorHandleForHeapStart();
    info.nextFreeIndex = 0;
    return true;
}

DescriptorHandle DescriptorHeapManager::Allocate(
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    UINT count)
{
    auto& info = descriptorHeaps[static_cast<size_t>(heapType)];

    if (info.nextFreeIndex + count > info.maxDescriptors)
        throw std::runtime_error("DescriptorHeapManager: heap exhausted");

    DescriptorHandle handle;
    handle.cpuHandle.ptr =
        info.cpuStart.ptr + SIZE_T(info.nextFreeIndex) * info.descriptorSize;

    if (info.isShaderVisible)
        handle.gpuHandle.ptr =
        info.gpuStart.ptr + UINT64(info.nextFreeIndex) * info.descriptorSize;
    else
        handle.gpuHandle.ptr = 0;

    handle.index = info.nextFreeIndex;
    info.nextFreeIndex += count;
    return handle;
}
