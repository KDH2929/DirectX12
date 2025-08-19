#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <stdexcept>

#include "DescriptorHandle.h"

using Microsoft::WRL::ComPtr;

/*
현재 ImGui 와 Sampler 관련 Heap 의 CPU/GPU Handle은 관리하지만 
CBV_SRV_UAV 와 RTV, DSV 의 Handle 은 관리는 안하고 있다.   
Handle 관리는 외부에서 하도록 하는 것도 괜찮을 거 같다.

SRV Handle은 현재 각자의 Texture들이 가지고 있고,   TextureManager가 관리한다.

CBV Handle은 현재 사용하지는 않는데, 상수버퍼를 Descriptor Table에 바인딩하여 사용하지 않고
Root Constant Buffer View 에 저장해서 사용하기 때문이다.
ex) SetGraphicsRootConstantBufferView (Root CBV)  

만약 DescriptorHeap 을 할당하여 상수버퍼를 쓴다면 현재로써는 각 GameObject들이 Handle 을 관리하도록 할 생각이다.

Sampler의 경우 추후 Sampler Manager 를 두어 여기서 관리하도록 하는 게 좋을 거 같다.

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
    UINT GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;      // 하나의 디스크립터 슬롯이 차지하는 바이트 크기 리턴

    ID3D12DescriptorHeap* GetImGuiSrvHeap() const { return imguiSrvHeap.Get(); }
    ID3D12DescriptorHeap* GetImGuiSamplerHeap() const { return imguiSamplerHeap.Get(); }

    D3D12_GPU_DESCRIPTOR_HANDLE CreateLinearWrapSampler(ID3D12Device* device);
    D3D12_GPU_DESCRIPTOR_HANDLE CreateLinearClampSampler(ID3D12Device* device);

    D3D12_GPU_DESCRIPTOR_HANDLE GetLinearWrapSamplerGpuHandle() const { return linearWrapSamplerHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetLinearClampSamplerGpuHandle() const { return linearClampSamplerHandle; }

    // RTV/DSV 뷰 생성
    bool CreateRenderTargetView(ID3D12Device* device, ID3D12Resource* resource, UINT heapIndex);    
    bool CreateDepthStencilView(ID3D12Device* device, ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* desc, UINT heapIndex);

    // RTV/DSV CPU 핸들 조회
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtvCpuHandle(UINT heapIndex) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvCpuHandle(UINT heapIndex) const;

    DescriptorHandle GetRtvHandle(UINT heapIndex) const;
    DescriptorHandle GetDsvHandle(UINT heapIndex) const;

private:
    struct DescriptorHeapInfo
    {
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;    // 실제 ID3D12DescriptorHeap 객체
        D3D12_CPU_DESCRIPTOR_HANDLE cpuStart;           // 힙의 첫 번째 CPU 핸들
        D3D12_GPU_DESCRIPTOR_HANDLE gpuStart;           // 힙의 첫 번째 GPU 핸들
        UINT descriptorSize = 0;                     // 한 슬롯당 바이트 오프셋(= GetDescriptorHandleIncrementSize)
        UINT maxDescriptors = 0;                     // NumDescriptors
        UINT nextFreeIndex = 0;                     // Allocate() 시 사용할 다음 인덱스
        bool isShaderVisible = false;                 // SHADER_VISIBLE 플래그 사용 여부
    };

    // index 0: CBV_SRV_UAV, 1: Sampler, 2: RTV, 3: DSV
    std::array<DescriptorHeapInfo, 4> descriptorHeaps;

    ComPtr<ID3D12DescriptorHeap> imguiSrvHeap;
    ComPtr<ID3D12DescriptorHeap> imguiSamplerHeap;

    bool CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, bool shaderVisible);

    D3D12_GPU_DESCRIPTOR_HANDLE linearWrapSamplerHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE linearClampSamplerHandle;
};