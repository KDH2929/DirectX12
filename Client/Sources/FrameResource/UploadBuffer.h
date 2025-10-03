#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cassert>
#include <cstring> // memcpy

using Microsoft::WRL::ComPtr;

// UploadBuffer: CPU→GPU 업로드용 버퍼 (T는 요소 타입)
template<typename T>
class UploadBuffer {
public:
    UploadBuffer(ID3D12Device* device, UINT count, bool constantBuffer)
        : count(count), isConstantBuffer(constantBuffer)
    {
        elementSize = sizeof(T);

        // 상수 버퍼는 256바이트 정렬 필요
        if (isConstantBuffer) {
            elementSize = (elementSize + 255) & ~255;
        }

        // 버퍼 리소스 생성
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width = UINT64(elementSize) * count;
        resDesc.Height = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&resource)
        );
        assert(SUCCEEDED(hr) && "UploadBuffer: CreateCommittedResource failed");

        // CPU에 매핑
        hr = resource->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
        assert(SUCCEEDED(hr) && "UploadBuffer: Map failed");
    }

    ~UploadBuffer() {
        if (resource && mappedData) {
            resource->Unmap(0, nullptr);
            mappedData = nullptr;
        }
    }

    UploadBuffer(const UploadBuffer&) = delete;
    UploadBuffer& operator=(const UploadBuffer&) = delete;

    // 요소 데이터 복사
    void CopyData(UINT index, const T& data) {
        assert(index < count);
        std::memcpy(reinterpret_cast<BYTE*>(mappedData) + index * elementSize, &data, sizeof(T));
    }

    // GPU 가상 주소
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(UINT index) const {
        assert(index < count);
        return resource->GetGPUVirtualAddress() + UINT64(index) * elementSize;
    }

private:
    ComPtr<ID3D12Resource> resource;
    BYTE* mappedData = nullptr; 
    UINT elementSize = 0;   
    UINT count = 0;             
    bool isConstantBuffer = false; \
};
