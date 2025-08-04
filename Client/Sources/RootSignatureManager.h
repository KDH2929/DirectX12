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


    ID3D12RootSignature* Get(const std::wstring& name) const;
    void Cleanup();

private:
    ID3D12Device* device; 
    std::unordered_map<std::wstring, ComPtr<ID3D12RootSignature>> signatureMap;
};