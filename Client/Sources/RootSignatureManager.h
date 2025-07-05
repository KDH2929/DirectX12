#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <unordered_map>

using namespace Microsoft::WRL;

class RootSignatureManager {
public:

    // explicit 키워드는 C 기반의 초기화 방지 (형변환을 방지함)
    explicit RootSignatureManager(ID3D12Device* device_);

    bool InitializeDescs();

    // v1
    bool Create(const std::wstring& name,
        const D3D12_ROOT_SIGNATURE_DESC& desc);

    // Versioned (1.1/1.2)
    bool Create(const std::wstring& name,
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc);

    // 생성된 루트 시그니처 반환
    ID3D12RootSignature* Get(const std::wstring& name) const;

    // 종료 시 캐시 및 device 해제
    void Cleanup();

private:
    ID3D12Device* device; 
    std::unordered_map<
        std::wstring,
        ComPtr<ID3D12RootSignature>
    > signatureMap;          // 이름 -> RootSignature 캐시
};