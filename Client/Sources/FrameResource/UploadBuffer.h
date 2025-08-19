#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <cassert>

using Microsoft::WRL::ComPtr;

// Microsoft Sample Code 및 Frank Luna DX12 Code 참고

// UploadBuffer: CPU→GPU 업로드용 버퍼 Wrapper
// T: 버퍼 요소 타입
template<typename T>
class UploadBuffer {
public:
    // device: D3D12 디바이스
    // elementCount: 총 요소 개수
    // isConstantBuffer: true면 256바이트 정렬
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer)
        : elementCount(elementCount)
        , isConstantBuffer(isConstantBuffer)
    {
        // 요소 크기 계산 (상수 버퍼면 256바이트 정렬)
        UINT rawSize = sizeof(T);
        if (isConstantBuffer) {
            elementSize = (rawSize + 255) & ~255;
        }
        else {
            elementSize = rawSize;
        }

        // 리소스 생성
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = UINT64(elementSize) * elementCount;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer)
        );
        assert(SUCCEEDED(hr) && "UploadBuffer: CreateCommittedResource failed");

        // 매핑
        D3D12_RANGE readRange = { 0, 0 }; // CPU가 읽지 않음
        hr = uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));
        assert(SUCCEEDED(hr) && "UploadBuffer: Map failed");
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

    ~UploadBuffer() {
        if (uploadBuffer && mappedData) {
            uploadBuffer->Unmap(0, nullptr);
            mappedData = nullptr;
        }
    }

    // 요소 데이터 복사
    void CopyData(UINT elementIndex, const T& data) {
        assert(elementIndex < elementCount);
        memcpy(&mappedData[elementIndex * (elementSize / sizeof(T))], &data, sizeof(T));
    }

    // GPU 가상 주소 반환
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(UINT elementIndex) const {
        assert(elementIndex < elementCount);
        return uploadBuffer->GetGPUVirtualAddress() + UINT64(elementIndex) * elementSize;
    }

    // 필요 시 Reset 호출
    void Reset() {
        // Upload heap은 재생성 없이 덮어쓰기만 하면 되므로 특별한 동작 없음
    }

private:
    ComPtr<ID3D12Resource> uploadBuffer;
    T* mappedData = nullptr;
    UINT                   elementSize = 0;
    UINT                   elementCount = 0;
    bool                   isConstantBuffer = false;
};

