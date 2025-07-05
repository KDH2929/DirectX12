#include "RootSignatureManager.h"
#include <stdexcept>
#include <format>
#include <assert.h>

RootSignatureManager::RootSignatureManager(ID3D12Device* device_)
    : device(device_)
{
}

bool RootSignatureManager::InitializeDescs()
{
    signatureMap.clear();

    // 1) TriangleRS ����
    {
        D3D12_ROOT_PARAMETER params[1] = {};

        // b0: ��-��-�������� CBV (VS ����)
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

    
    // 2) �� ����� PhongRS ����
    {
        // b0~b3: CBVs for MVP, Lighting, Material, Global
        D3D12_ROOT_PARAMETER params[6] = {};          //-- �ʱ�ȭ
        for (int i = 0; i < 4; ++i) {
            params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            params[i].Descriptor.ShaderRegister = i;      // b0, b1, b2, b3
            params[i].Descriptor.RegisterSpace = 0;
            params[i].ShaderVisibility =
                (i == 0 ? D3D12_SHADER_VISIBILITY_VERTEX
                    : D3D12_SHADER_VISIBILITY_PIXEL);
        }

        // t0, t1 -- �˺��� + ���  �� �� ���̺� 2 �� SRV
        D3D12_DESCRIPTOR_RANGE srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 2;        // 1  ��  2
        srvRange.BaseShaderRegister = 0;      // t0
        srvRange.RegisterSpace = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // s0  (���÷� ���̺��� �״��)
        D3D12_DESCRIPTOR_RANGE sampRange = {};
        sampRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        sampRange.NumDescriptors = 1;
        sampRange.BaseShaderRegister = 0;     // s0
        sampRange.RegisterSpace = 0;
        sampRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // param[4] : SRV table (t0, t1)
        params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[4].DescriptorTable.NumDescriptorRanges = 1;
        params[4].DescriptorTable.pDescriptorRanges = &srvRange;
        params[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // param[5] : Sampler table (s0)
        params[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[5].DescriptorTable.NumDescriptorRanges = 1;
        params[5].DescriptorTable.pDescriptorRanges = &sampRange;
        params[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = _countof(params);
        desc.pParameters = params;
        desc.NumStaticSamplers = 0;
        desc.pStaticSamplers = nullptr;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Create(L"PhongRS", desc);
    }


    // 3) PBR ����� PbrRS ����
    {
        // b0~b3: CBVs for MVP, Lighting, Material, Global
        D3D12_ROOT_PARAMETER params[6] = {};
        for (int i = 0; i < 4; ++i) {
            params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            params[i].Descriptor.ShaderRegister = i;  // b0, b1, b2, b3
            params[i].Descriptor.RegisterSpace = 0;
            params[i].ShaderVisibility = (i == 0)
                ? D3D12_SHADER_VISIBILITY_VERTEX
                : D3D12_SHADER_VISIBILITY_PIXEL;
        }

        // t0~t3: SRV ���̺� (�˺���, ���, ��Ż��, �����Ͻ�)
        D3D12_DESCRIPTOR_RANGE srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 4;      // 4 textures
        srvRange.BaseShaderRegister = 0;      // t0
        srvRange.RegisterSpace = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[4].DescriptorTable.NumDescriptorRanges = 1;
        params[4].DescriptorTable.pDescriptorRanges = &srvRange;
        params[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // s0: ���÷� ���̺�
        D3D12_DESCRIPTOR_RANGE sampRange = {};
        sampRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        sampRange.NumDescriptors = 1;
        sampRange.BaseShaderRegister = 0; // s0
        sampRange.RegisterSpace = 0;
        sampRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[5].DescriptorTable.NumDescriptorRanges = 1;
        params[5].DescriptorTable.pDescriptorRanges = &sampRange;
        params[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // ��Ʈ �ñ״�ó ����(v1.0)
        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = _countof(params);
        desc.pParameters = params;
        desc.NumStaticSamplers = 0;
        desc.pStaticSamplers = nullptr;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // ���� ȣ��
        if (!Create(L"PbrRS", desc))
            throw std::runtime_error("RootSignatureManager::Create(\"PbrRS\") failed");
    }


    return true;
}


bool RootSignatureManager::Create(
    const std::wstring& name,
    const D3D12_ROOT_SIGNATURE_DESC& desc)
{
    if (signatureMap.count(name))
        return true;  // �̹� ������

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

bool RootSignatureManager::Create(const std::wstring& name,
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& descIn)
{
    if (signatureMap.contains(name))
        return true;

    // 1) �ϵ��� v1.1 (bindless) ��Ʈ �ñ״�ó�� �����ϴ��� Ȯ��
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{ D3D_ROOT_SIGNATURE_VERSION_1_1 };
    bool supportV11 = SUCCEEDED(device->CheckFeatureSupport(
        D3D12_FEATURE_ROOT_SIGNATURE,
        &featureData,
        sizeof(featureData)))
        && featureData.HighestVersion >= D3D_ROOT_SIGNATURE_VERSION_1_1;

    // 2) desc ���� ��, ���� �� �ϸ� ���� �ε��� �÷��� ����
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = descIn;
    if (!supportV11) {
        OutputDebugStringA("Bindless not supported; stripping direct-index flags\n");
        // v1.0 ��η� ���� ����
        desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
        // Desc_1_0�� �Ķ���͡�flags�� ���; ���� �ε��� �÷��� ����
        desc.Desc_1_0.Flags &= ~(D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
            | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED);
    }

    // 3) Serialize
    ComPtr<ID3DBlob> blob, error;
    HRESULT hr = D3D12SerializeVersionedRootSignature(&desc, &blob, &error);
    if (FAILED(hr)) {
        if (error) OutputDebugStringA((char*)error->GetBufferPointer());
        return false;
    }

    // 4) CreateRootSignature ȣ��
    //    v1.1 ���� �� ID3D12Device1, �ƴϸ� ID3D12Device
    ComPtr<ID3D12RootSignature> signature;
    if (supportV11) {
        ComPtr<ID3D12Device1> device1;
        if (FAILED(device->QueryInterface(IID_PPV_ARGS(&device1))))
            return false;
        hr = device1->CreateRootSignature(
            0, blob->GetBufferPointer(), blob->GetBufferSize(),
            IID_PPV_ARGS(&signature));
    }
    else {
        hr = device->CreateRootSignature(
            0, blob->GetBufferPointer(), blob->GetBufferSize(),
            IID_PPV_ARGS(&signature));
    }
    if (FAILED(hr)) {
        OutputDebugStringA("CreateRootSignature failed\n");
        return false;
    }

    signatureMap.emplace(name, signature);
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
