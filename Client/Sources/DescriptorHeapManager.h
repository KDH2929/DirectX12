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

/*
���� ImGui �� Sampler ���� Heap �� CPU/GPU Handle�� ���������� 
CBV_SRV_UAV �� RTV, DSV �� Handle �� ������ ���ϰ� �ִ�.   
Handle ������ �ܺο��� �ϵ��� �ϴ� �͵� ������ �� ����.

SRV Handle�� ���� ������ Texture���� ������ �ְ�,   TextureManager�� �����Ѵ�.

CBV Handle�� ���� ��������� �ʴµ�, ������۸� Descriptor Table�� ���ε��Ͽ� ������� �ʰ�
Root Constant Buffer View �� �����ؼ� ����ϱ� �����̴�.
ex) SetGraphicsRootConstantBufferView (Root CBV)  

���� DescriptorHeap �� �Ҵ��Ͽ� ������۸� ���ٸ� ����ν�� �� GameObject���� Handle �� �����ϵ��� �� �����̴�.

Sampler�� ��� ���� Sampler Manager �� �ξ� ���⼭ �����ϵ��� �ϴ� �� ���� �� ����.

*/

// ---------------------------------------------------------------------------
// DescriptorHeapManager
// Manages four descriptor heaps and provides allocation:
//   - CBV_SRV_UAV
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
    

    // ���� ������. ���� ��Ƽ������ ������ ���� �� �ʿ�������?
    void BeginFrame(UINT backBufferIndex);

    DescriptorHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count = 1);

    ID3D12DescriptorHeap* GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
    UINT GetStride(D3D12_DESCRIPTOR_HEAP_TYPE type) const;      // �ϳ��� ��ũ���� ������ �����ϴ� ����Ʈ ũ�� ����

    ID3D12DescriptorHeap* GetImGuiSrvHeap() const { return imguiSrvHeap.Get(); }
    ID3D12DescriptorHeap* GetImGuiSamplerHeap() const { return imguiSamplerHeap.Get(); }

    D3D12_GPU_DESCRIPTOR_HANDLE CreateWrapSampler(ID3D12Device* device);
    D3D12_GPU_DESCRIPTOR_HANDLE CreateClampSampler(ID3D12Device* device);

    D3D12_GPU_DESCRIPTOR_HANDLE GetWrapSamplerHandle() const { return wrapSamplerHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetClampSamplerHandle() const { return clampSamplerHandle; }

    // RTV/DSV �� ����
    bool CreateRenderTargetView(ID3D12Device* device, ID3D12Resource* resource, UINT heapIndex);
    bool CreateDepthStencilView(ID3D12Device* device, ID3D12Resource* resource, UINT heapIndex);

    // RTV/DSV CPU �ڵ� ��ȸ
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(UINT heapIndex) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle(UINT heapIndex) const;

private:
    struct HeapInfo
    {
        ComPtr<ID3D12DescriptorHeap> heap;
        D3D12_CPU_DESCRIPTOR_HANDLE  cpuBase{};
        D3D12_GPU_DESCRIPTOR_HANDLE  gpuBase{};
        UINT stride = 0;                 // stride: �ش� ���� �� ��ũ���� ������ �����ϴ� ����Ʈ ũ��
        UINT capacity = 0;              // NumDescriptors �� ������ �ִ� ���� ����
        UINT cursor = 0;                // Allocate ȣ�� �� �������� �Ҵ��� ���� �ε���
        bool shaderVisible = false;
    };

    // heaps[0] �� CBV_SRV_UAV �� ���� (stride, capacity, cursor��)
    // heaps[1] �� Sampler �� ����
    // heaps[2] �� RTV �� ����
    // heaps[3] �� DSV �� ����
    std::array<HeapInfo, 4> heaps{};


    UINT sliceCount = 1;
    UINT sliceSize = 0;

    ComPtr<ID3D12DescriptorHeap> imguiSrvHeap;
    ComPtr<ID3D12DescriptorHeap> imguiSamplerHeap;

    bool CreateHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, bool shaderVisible);

    D3D12_GPU_DESCRIPTOR_HANDLE wrapSamplerHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE clampSamplerHandle;
};