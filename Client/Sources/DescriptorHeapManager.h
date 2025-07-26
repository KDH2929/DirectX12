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

    bool InitializeImGuiHeaps(ID3D12Device* device, UINT imguiSrvCount = 4, UINT imguiSamplerCount = 1);
    

    // 아직 사용안함. 추후 멀티스레드 렌더링 구현 시 필요할지도?
    void BeginFrame(UINT backBufferIndex);

    DescriptorHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count = 1);

    ID3D12DescriptorHeap* GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
    UINT GetStride(D3D12_DESCRIPTOR_HEAP_TYPE type) const;      // 하나의 디스크립터 슬롯이 차지하는 바이트 크기 리턴

    ID3D12DescriptorHeap* GetImGuiSrvHeap() const { return imguiSrvHeap.Get(); }
    ID3D12DescriptorHeap* GetImGuiSamplerHeap() const { return imguiSamplerHeap.Get(); }

    D3D12_GPU_DESCRIPTOR_HANDLE CreateWrapSampler(ID3D12Device* device);
    D3D12_GPU_DESCRIPTOR_HANDLE CreateClampSampler(ID3D12Device* device);

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
    struct HeapInfo
    {
        ComPtr<ID3D12DescriptorHeap> heap;
        D3D12_CPU_DESCRIPTOR_HANDLE  cpuBase{};
        D3D12_GPU_DESCRIPTOR_HANDLE  gpuBase{};
        UINT stride = 0;                 // stride: 해당 힙의 한 디스크립터 슬롯이 차지하는 바이트 크기
        UINT capacity = 0;              // NumDescriptors 로 설정한 최대 슬롯 개수
        UINT cursor = 0;                // Allocate 호출 시 다음으로 할당할 슬롯 인덱스
        bool shaderVisible = false;
    };

    // heaps[0] → CBV_SRV_UAV 힙 정보 (stride, capacity, cursor…)
    // heaps[1] → Sampler 힙 정보
    // heaps[2] → RTV 힙 정보
    // heaps[3] → DSV 힙 정보
    std::array<HeapInfo, 4> heaps{};


    UINT sliceCount = 1;
    UINT sliceSize = 0;

    ComPtr<ID3D12DescriptorHeap> imguiSrvHeap;
    ComPtr<ID3D12DescriptorHeap> imguiSamplerHeap;

    bool CreateHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, bool shaderVisible);

    D3D12_GPU_DESCRIPTOR_HANDLE linearWrapSamplerHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE linearClampSamplerHandle;
};