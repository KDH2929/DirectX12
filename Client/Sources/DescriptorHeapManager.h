#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <stdexcept>

#include "DescriptorHandle.h"

using Microsoft::WRL::ComPtr;

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

    bool InitializeImGuiDescriptorHeaps(ID3D12Device* device, UINT imguiSrvCount = 4, UINT imguiSamplerCount = 1);

    DescriptorHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count = 1);

    ID3D12DescriptorHeap* GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
    UINT GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;      // �ϳ��� ��ũ���� ������ �����ϴ� ����Ʈ ũ�� ����

    ID3D12DescriptorHeap* GetImGuiSrvHeap() const { return imguiSrvHeap.Get(); }
    ID3D12DescriptorHeap* GetImGuiSamplerHeap() const { return imguiSamplerHeap.Get(); }

    D3D12_GPU_DESCRIPTOR_HANDLE CreateLinearWrapSampler(ID3D12Device* device);
    D3D12_GPU_DESCRIPTOR_HANDLE CreateLinearClampSampler(ID3D12Device* device);

    D3D12_GPU_DESCRIPTOR_HANDLE GetLinearWrapSamplerGpuHandle() const { return linearWrapSamplerHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetLinearClampSamplerGpuHandle() const { return linearClampSamplerHandle; }

    // RTV/DSV �� ����
    bool CreateRenderTargetView(ID3D12Device* device, ID3D12Resource* resource, UINT heapIndex);    
    bool CreateDepthStencilView(ID3D12Device* device, ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* desc, UINT heapIndex);

    // RTV/DSV CPU �ڵ� ��ȸ
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtvCpuHandle(UINT heapIndex) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvCpuHandle(UINT heapIndex) const;

    DescriptorHandle GetRtvHandle(UINT heapIndex) const;
    DescriptorHandle GetDsvHandle(UINT heapIndex) const;

private:
    struct DescriptorHeapInfo
    {
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;    // ���� ID3D12DescriptorHeap ��ü
        D3D12_CPU_DESCRIPTOR_HANDLE cpuStart;           // ���� ù ��° CPU �ڵ�
        D3D12_GPU_DESCRIPTOR_HANDLE gpuStart;           // ���� ù ��° GPU �ڵ�
        UINT descriptorSize = 0;                     // �� ���Դ� ����Ʈ ������(= GetDescriptorHandleIncrementSize)
        UINT maxDescriptors = 0;                     // NumDescriptors
        UINT nextFreeIndex = 0;                     // Allocate() �� ����� ���� �ε���
        bool isShaderVisible = false;                 // SHADER_VISIBLE �÷��� ��� ����
    };

    // index 0: CBV_SRV_UAV, 1: Sampler, 2: RTV, 3: DSV
    std::array<DescriptorHeapInfo, 4> descriptorHeaps;

    ComPtr<ID3D12DescriptorHeap> imguiSrvHeap;
    ComPtr<ID3D12DescriptorHeap> imguiSamplerHeap;

    bool CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, bool shaderVisible);

    D3D12_GPU_DESCRIPTOR_HANDLE linearWrapSamplerHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE linearClampSamplerHandle;
};