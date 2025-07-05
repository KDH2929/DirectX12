#pragma once
#include <memory>
#include <string>
#include <wrl/client.h>
#include <d3d12.h>

class Renderer;

/* -----------------------------------------------------------
 * GPU-텍스처 + SRV 핸들 보유
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

    // 핸들 설정 (TextureManager 가 SRV를 만들고 호출)
    void SetDescriptorHandles(D3D12_CPU_DESCRIPTOR_HANDLE cpu,
        D3D12_GPU_DESCRIPTOR_HANDLE gpu,
        UINT index);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> texture;      // 실제 GPU 리소스
    std::wstring  name;                                  // 파일 경로(캐시 키)

    // SRV 위치
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ 0 };
    UINT descriptorIndex{ UINT(-1) };
};
