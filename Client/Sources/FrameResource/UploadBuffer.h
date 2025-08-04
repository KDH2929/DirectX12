#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <cassert>

using Microsoft::WRL::ComPtr;

// Microsoft Sample Code �� Frank Luna DX12 Code ����

// UploadBuffer: CPU��GPU ���ε�� ���� Wrapper
// T: ���� ��� Ÿ��
template<typename T>
class UploadBuffer {
public:
    // device: D3D12 ����̽�
    // elementCount: �� ��� ����
    // isConstantBuffer: true�� 256����Ʈ ����
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer)
        : elementCount(elementCount)
        , isConstantBuffer(isConstantBuffer)
    {
        // ��� ũ�� ��� (��� ���۸� 256����Ʈ ����)
        UINT rawSize = sizeof(T);
        if (isConstantBuffer) {
            elementSize = (rawSize + 255) & ~255;
        }
        else {
            elementSize = rawSize;
        }

        // ���ҽ� ����
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

        // ����
        D3D12_RANGE readRange = { 0, 0 }; // CPU�� ���� ����
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

    // ��� ������ ����
    void CopyData(UINT elementIndex, const T& data) {
        assert(elementIndex < elementCount);
        memcpy(&mappedData[elementIndex * (elementSize / sizeof(T))], &data, sizeof(T));
    }

    // GPU ���� �ּ� ��ȯ
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(UINT elementIndex) const {
        assert(elementIndex < elementCount);
        return uploadBuffer->GetGPUVirtualAddress() + UINT64(elementIndex) * elementSize;
    }

    // �ʿ� �� Reset ȣ��
    void Reset() {
        // Upload heap�� ����� ���� ����⸸ �ϸ� �ǹǷ� Ư���� ���� ����
    }

private:
    ComPtr<ID3D12Resource> uploadBuffer;
    T* mappedData = nullptr;
    UINT                   elementSize = 0;
    UINT                   elementCount = 0;
    bool                   isConstantBuffer = false;
};

