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
// Owns all four physical descriptor heaps required by D3D12.
// Provides type-aware Allocate() and frame-slice management for the
// CBV_SRV_UAV heap -- everything in one place, zero extra classes.
// ---------------------------------------------------------------------------
class DescriptorHeapManager
{
public:
    bool Initialize(ID3D12Device* device,
        UINT cbvSrvUavCount = 40'000,
        UINT samplerCount = 128,
        UINT rtvCount = 8,
        UINT dsvCount = 4,
        UINT backBufferCount = 3);

    void BeginFrame(UINT frameIndex);

    DescriptorHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count = 1);

    // Getters ---------------------------------------------------------------
    ID3D12DescriptorHeap* GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
    {
        return heaps[static_cast<size_t>(type)].heap.Get();
    }

    UINT GetStride(D3D12_DESCRIPTOR_HEAP_TYPE type) const
    {
        return heaps[static_cast<size_t>(type)].stride;
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

    std::array<HeapInfo, 4> heaps{};  // indexed by heap enum value

    UINT sliceCount = 1;   // swap-chain buffer count
    UINT sliceSize = 0;   // per-frame capacity for CBV_SRV_UAV heap

    bool CreateHeap(ID3D12Device* device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        UINT count,
        bool shaderVisible);
};