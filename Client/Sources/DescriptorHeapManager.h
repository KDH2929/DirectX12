#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

struct DescriptorHandle
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpu{ 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE gpu{ 0 };
    UINT index = 0;   // offset inside the heap
};

// ---------------------------------------------------------------------------
// DescriptorHeapManager
// Manages four descriptor heaps and provides allocation:
//   - CBV_SRV_UAV (frame-sliced)
//   - SAMPLER
//   - RTV
//   - DSV
// Also creates a dedicated SRV heap for ImGui resources.
// ---------------------------------------------------------------------------
class DescriptorHeapManager
{
public:

    bool Initialize(ID3D12Device* device,
        UINT cbvSrvUavCount = 40000,
        UINT samplerCount = 128,
        UINT rtvCount = 8,
        UINT dsvCount = 4,
        UINT backBufferCount = 3);

    bool InitializeImGuiHeaps(ID3D12Device* device, UINT imguiSrvCount = 4, UINT imguiSamplerCount = 1);
    

    // 아직 사용안함. 추후 멀티스레드 렌더링 구현 시 필요할지도?
    void BeginFrame(UINT frameIndex);

    DescriptorHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count = 1);

    ID3D12DescriptorHeap* GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
    UINT GetStride(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

    ID3D12DescriptorHeap* GetImGuiSrvHeap() const { return imguiSrvHeap.Get(); }
    ID3D12DescriptorHeap* GetImGuiSamplerHeap() const { return imguiSamplerHeap.Get(); }

    D3D12_GPU_DESCRIPTOR_HANDLE CreateWrapSampler(ID3D12Device* device);
    D3D12_GPU_DESCRIPTOR_HANDLE CreateClampSampler(ID3D12Device* device);

    D3D12_GPU_DESCRIPTOR_HANDLE GetWrapSamplerHandle() const {
        return wrapSamplerHandle;
    }
    D3D12_GPU_DESCRIPTOR_HANDLE GetClampSamplerHandle() const {
        return clampSamplerHandle;
    }


private:
    struct HeapInfo
    {
        ComPtr<ID3D12DescriptorHeap> heap;
        D3D12_CPU_DESCRIPTOR_HANDLE  cpuBase{};
        D3D12_GPU_DESCRIPTOR_HANDLE  gpuBase{};
        UINT stride = 0;
        UINT capacity = 0;
        UINT cursor = 0;
        bool shaderVisible = false;
    };

    std::array<HeapInfo, 4> heaps{};
    UINT sliceCount = 1;
    UINT sliceSize = 0;

    ComPtr<ID3D12DescriptorHeap> imguiSrvHeap;
    ComPtr<ID3D12DescriptorHeap> imguiSamplerHeap;

    bool CreateHeap(ID3D12Device* device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        UINT count,
        bool shaderVisible);

    D3D12_GPU_DESCRIPTOR_HANDLE wrapSamplerHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE clampSamplerHandle;
};