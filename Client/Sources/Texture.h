#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <string>

class Texture
{
public:
    Texture() = default;
    ~Texture() = default;

    bool LoadFromFile(ID3D12Device* device,
        ID3D12GraphicsCommandList* copyCommandList,
        const std::wstring& filePath);

    void SetDescriptorHandles(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    ID3D12Resource* GetResource()   const;
    D3D12_GPU_DESCRIPTOR_HANDLE    GetGpuHandle()  const;
    D3D12_CPU_DESCRIPTOR_HANDLE    GetCpuHandle()  const;
    const std::wstring& GetName()       const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> texture;        // Default-heap texture
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;   // Upload-heap buffer

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};   // shader-resource-view À§Ä¡
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};

    std::wstring name;
};