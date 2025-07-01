#include "RootSignatureManager.h"
#include <stdexcept>

RootSignatureManager::RootSignatureManager(ID3D12Device* device_)
    : device(device_)
{
}

bool RootSignatureManager::InitializeDescs()
{
    signatureMap.clear();

    //
    // 1) TriangleRS 생성
    //
    {
        D3D12_ROOT_PARAMETER params[1] = {};

        // b0: 모델-뷰-프로젝션 CBV (VS 전용)
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0;
        params[0].Descriptor.RegisterSpace = 0;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = _countof(params);
        desc.pParameters = params;
        desc.NumStaticSamplers = 0;
        desc.pStaticSamplers = nullptr;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Create(L"TriangleRS", desc);
    }

    //
    // 2) 퐁 조명용 PhongRS 생성
    //
    {
        // b0~b3: CBVs for MVP, Lighting, Material, Global 
        D3D12_ROOT_PARAMETER params[6] = { /* b0~b3 */ };

        for (int i = 0; i < 4; ++i) {
            params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            params[i].Descriptor.ShaderRegister = i;    // b0, b1, b2, b3
            params[i].Descriptor.RegisterSpace = 0;
            params[i].ShaderVisibility =
                (i == 0 ? D3D12_SHADER_VISIBILITY_VERTEX
                    : D3D12_SHADER_VISIBILITY_PIXEL);
        }

        // t0 + s0용 디스크립터 테이블
        // ranges[0] = SRV (CBV/SRV/UAV 힙)
        D3D12_DESCRIPTOR_RANGE srvRange = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0 };

        // ranges[1] = Sampler (Sampler 힙)  은 수행시 에러..    같은 table 불가
        // ---> 별도 root parameter
        D3D12_DESCRIPTOR_RANGE sampRange = { D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,1,0 };


        // param[4] : SRV table
        params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[4].DescriptorTable = { 1, &srvRange };
        params[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // param[5] : Sampler table
        params[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[5].DescriptorTable = { 1, &sampRange };
        params[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = _countof(params);
        desc.pParameters = params;
        desc.NumStaticSamplers = 0;
        desc.pStaticSamplers = nullptr;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Create(L"PhongRS", desc);
    }

    return true;
}


bool RootSignatureManager::Create(
    const std::wstring& name,
    const D3D12_ROOT_SIGNATURE_DESC& desc)
{
    if (signatureMap.count(name))
        return true;  // 이미 생성됨

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3D12SerializeRootSignature(
        &desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &blob,
        &error
    );
    if (FAILED(hr)) {
        if (error)
            OutputDebugStringA(
                static_cast<const char*>(error->GetBufferPointer())
            );
        return false;
    }

    ComPtr<ID3D12RootSignature> rootSignature;
    hr = device->CreateRootSignature(
        0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)
    );
    if (FAILED(hr))
        return false;

    signatureMap[name] = rootSignature;
    return true;
}

ID3D12RootSignature* RootSignatureManager::Get(
    const std::wstring& name) const
{
    auto it = signatureMap.find(name);
    return it != signatureMap.end() ? it->second.Get() : nullptr;
}

void RootSignatureManager::Cleanup() {
    signatureMap.clear();
    device = nullptr;
}
