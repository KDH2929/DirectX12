#pragma once
#include <memory>
#include <string>
#include <wrl/client.h>
#include <d3d12.h>

class Renderer;

/* -----------------------------------------------------------
 * GPU-�ؽ�ó + SRV �ڵ� ����
 * ----------------------------------------------------------*/
class Texture
{
public:
    Texture() = default;

    bool LoadFromFile(Renderer* renderer,
        const std::wstring& filePath);

    ID3D12Resource* GetResource()       const;
    D3D12_GPU_DESCRIPTOR_HANDLE  GetGpuHandle()      const;
    D3D12_CPU_DESCRIPTOR_HANDLE  GetCpuHandle()      const;
    UINT                         GetDescriptorIndex()const;
    const std::wstring& GetName()           const;

    // �ڵ� ���� (TextureManager �� SRV�� ����� ȣ��)
    void SetDescriptorHandles(D3D12_CPU_DESCRIPTOR_HANDLE cpu,
        D3D12_GPU_DESCRIPTOR_HANDLE gpu,
        UINT index);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> texture;      // ���� GPU ���ҽ�
    std::wstring  name;                                  // ���� ���(ĳ�� Ű)

    // SRV ��ġ
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ 0 };
    UINT descriptorIndex{ UINT(-1) };
};
