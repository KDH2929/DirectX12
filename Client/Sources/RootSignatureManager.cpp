#include "RootSignatureManager.h"
#include "ShadowMap.h"
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

    // 3) PBR�� ��Ʈ �ñ״�ó PbrRS ����
    {
        // b0~b3: CBV (b0=MVP, b1=Lighting, b2=Material, b3=Global)
        D3D12_ROOT_PARAMETER params[9] = {};
        for (UINT i = 0; i < 4; ++i)
        {
            params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            params[i].Descriptor.ShaderRegister = i;  // b0, b1, b2, b3
            params[i].Descriptor.RegisterSpace = 0;
            params[i].ShaderVisibility = (i == 0)
                ? D3D12_SHADER_VISIBILITY_VERTEX
                : D3D12_SHADER_VISIBILITY_PIXEL;
        }

        // Material(4) Descriptor Table �� root 4
        D3D12_DESCRIPTOR_RANGE matRange{};
        matRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        matRange.NumDescriptors = 4;   // t0~t3
        matRange.BaseShaderRegister = 0;
        matRange.RegisterSpace = 0;
        matRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[4].DescriptorTable.NumDescriptorRanges = 1;
        params[4].DescriptorTable.pDescriptorRanges = &matRange;
        params[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // IrradianceCube �� root 5 (t4)
        D3D12_DESCRIPTOR_RANGE irrRange = matRange;
        irrRange.NumDescriptors = 1;
        irrRange.BaseShaderRegister = 4;  // t4

        params[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[5].DescriptorTable.NumDescriptorRanges = 1;
        params[5].DescriptorTable.pDescriptorRanges = &irrRange;
        params[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // PrefilteredCube �� root 6 (t5)
        D3D12_DESCRIPTOR_RANGE prefRange = matRange;
        prefRange.NumDescriptors = 1;
        prefRange.BaseShaderRegister = 5;  // t5

        params[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[6].DescriptorTable.NumDescriptorRanges = 1;
        params[6].DescriptorTable.pDescriptorRanges = &prefRange;
        params[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // BRDF LUT �� root 7 (t6)
        D3D12_DESCRIPTOR_RANGE lutRange = matRange;
        lutRange.NumDescriptors = 1;
        lutRange.BaseShaderRegister = 6;  // t6

        params[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[7].DescriptorTable.NumDescriptorRanges = 1;
        params[7].DescriptorTable.pDescriptorRanges = &lutRange;
        params[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        
        // ShadowMap �� root 8
        /*
        D3D12_DESCRIPTOR_RANGE shadowRange{};
        shadowRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        shadowRange.NumDescriptors = MAX_SHADOW_DSV_COUNT; // t7 ~ t7+N-1
        shadowRange.BaseShaderRegister = 7;                   // t7
        shadowRange.RegisterSpace = 0;
        shadowRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[8].DescriptorTable.NumDescriptorRanges = 1;
        params[8].DescriptorTable.pDescriptorRanges = &shadowRange;
        params[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        */

        // Sampler ���̺� �� root 9 (s0~s1)
        D3D12_DESCRIPTOR_RANGE sampRange{};
        sampRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        sampRange.NumDescriptors = 2;      // s0, s1
        sampRange.BaseShaderRegister = 0;     // s0 ����
        sampRange.RegisterSpace = 0;
        sampRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[8].DescriptorTable.NumDescriptorRanges = 1;
        params[8].DescriptorTable.pDescriptorRanges = &sampRange;
        params[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


        // static sampler for shadow comparison (s2)
        static D3D12_STATIC_SAMPLER_DESC staticShadowSampler{};
        staticShadowSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        staticShadowSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticShadowSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticShadowSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticShadowSampler.MipLODBias = 0;
        staticShadowSampler.MaxAnisotropy = 1;      // comparison sampler������ 1�̻��� �ƴϸ� ����
        staticShadowSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        staticShadowSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        staticShadowSampler.MinLOD = 0;
        staticShadowSampler.MaxLOD = D3D12_FLOAT32_MAX;
        staticShadowSampler.ShaderRegister = 2;  // s2
        staticShadowSampler.RegisterSpace = 0;
        staticShadowSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


        // ��Ʈ �ñ״�ó Desc ����
        D3D12_ROOT_SIGNATURE_DESC desc{};
        desc.NumParameters = _countof(params);
        desc.pParameters = params;
        desc.NumStaticSamplers = 0;
        desc.pStaticSamplers = nullptr;  //&staticShadowSampler;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // 8) ���� ȣ��
        if (!Create(L"PbrRS", desc))
            throw std::runtime_error("RootSignatureManager::Create(\"PbrRS\") failed");
    }


    // 4) Skybox(ť���)�� ��Ʈ �ñ״�ó
    {
        // b0 : CB_MVP (world-view-proj)
        // t0 : ť��� SRV
        // s0 : ���÷�
        D3D12_ROOT_PARAMETER params[3] = {};

        // CBV b0
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0;
        params[0].Descriptor.RegisterSpace = 0;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // SRV ���̺� t0
        D3D12_DESCRIPTOR_RANGE srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 1;   // ť��� �� ��
        srvRange.BaseShaderRegister = 0;   // t0
        srvRange.RegisterSpace = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[1].DescriptorTable.NumDescriptorRanges = 1;
        params[1].DescriptorTable.pDescriptorRanges = &srvRange;
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // ���÷� ���̺� s0
        D3D12_DESCRIPTOR_RANGE samplerRange = {};
        samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        samplerRange.NumDescriptors = 1;
        samplerRange.BaseShaderRegister = 0;   // s0
        samplerRange.RegisterSpace = 0;
        samplerRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[2].DescriptorTable.NumDescriptorRanges = 1;
        params[2].DescriptorTable.pDescriptorRanges = &samplerRange;
        params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // ��Ʈ �ñ״�ó ����
        D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
        rsDesc.NumParameters = _countof(params);
        rsDesc.pParameters = params;
        rsDesc.NumStaticSamplers = 0;
        rsDesc.pStaticSamplers = nullptr;
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // ���� ȣ�� ("SkyboxRS"�� �ĺ�)
        if (!Create(L"SkyboxRS", rsDesc))
            throw std::runtime_error("RootSignatureManager::Create(\"SkyboxRS\") failed");
    }


    // DebugNormal RS
    {
        // 1) �Ķ���� �迭: CBV b0 �ϳ�
        D3D12_ROOT_PARAMETER debugParams[1] = {};
        debugParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        debugParams[0].Descriptor.ShaderRegister = 0; // b0
        debugParams[0].Descriptor.RegisterSpace = 0;
        debugParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // VS/GS/PS ���� ���

        // 2) ��Ʈ �ñ״�ó Desc
        D3D12_ROOT_SIGNATURE_DESC debugRSDesc = {};
        debugRSDesc.NumParameters = 1;
        debugRSDesc.pParameters = debugParams;
        debugRSDesc.NumStaticSamplers = 0;
        debugRSDesc.pStaticSamplers = nullptr;
        debugRSDesc.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // 3) ����
        Create(L"DebugNormalRS", debugRSDesc);
    }


    // PostProcess RS
    {
        //  b0 : �Ķ���� CBV ( PostProcess �ɼǵ� )
        //  t0 : �Է� �ؽ�ó (SceneColor, �Ǵ� ���� SRV ����)
        //  s0 : �������÷� (linear clamp)
        D3D12_ROOT_PARAMETER postParams[2] = {};

        // b0 �� CBV
        postParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        postParams[0].Descriptor.ShaderRegister = 0;      // b0
        postParams[0].Descriptor.RegisterSpace = 0;
        postParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // t0 �� SRV ���̺�
        D3D12_DESCRIPTOR_RANGE descTable = {};
        descTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descTable.NumDescriptors = 1;
        descTable.BaseShaderRegister = 0;      // t0
        descTable.RegisterSpace = 0;
        descTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        postParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        postParams[1].DescriptorTable.NumDescriptorRanges = 1;
        postParams[1].DescriptorTable.pDescriptorRanges = &descTable;
        postParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Static Sampler (s0)
        D3D12_STATIC_SAMPLER_DESC postSampler = {};
        postSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        postSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        postSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        postSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        postSampler.ShaderRegister = 0;  // s0
        postSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC postDesc = {};
        postDesc.NumParameters = _countof(postParams);
        postDesc.pParameters = postParams;
        postDesc.NumStaticSamplers = 1;
        postDesc.pStaticSamplers = &postSampler;
        postDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // ����
        Create(L"PostProcessRS", postDesc);
    }

    
    // ShadowMapPass RS :  ���̰��� �����
    {
        // b0 : CBV (world, lightViewProj)
        D3D12_ROOT_PARAMETER shadowParams[1] = {};

        shadowParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        shadowParams[0].Descriptor.ShaderRegister = 0;     // b0
        shadowParams[0].Descriptor.RegisterSpace = 0;
        shadowParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_ROOT_SIGNATURE_DESC shadowDesc = {};
        shadowDesc.NumParameters = _countof(shadowParams);
        shadowDesc.pParameters = shadowParams;
        shadowDesc.NumStaticSamplers = 0;
        shadowDesc.pStaticSamplers = nullptr;
        shadowDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Create(L"ShadowMapPassRS", shadowDesc);
    }



    // CloudVolume �������� ��Ʈ �ñ״�ó
    {
        // b1 : CB_CloudParameters
        // t2 : (����) 3D ������ ���� �ؽ�ó SRV
        D3D12_ROOT_PARAMETER params[2] = {};

        // CBV b1 �� CloudParameters
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 1;    // b1
        params[0].Descriptor.RegisterSpace = 0;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // SRV ���̺� t2 �� Noise Texture
        D3D12_DESCRIPTOR_RANGE srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 1;    // t2 �ϳ�
        srvRange.BaseShaderRegister = 2;    // t2
        srvRange.RegisterSpace = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[1].DescriptorTable.NumDescriptorRanges = 1;
        params[1].DescriptorTable.pDescriptorRanges = &srvRange;
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // 2) Static Sampler (NoiseSampler : register(s2))
        D3D12_STATIC_SAMPLER_DESC staticSampler = {};
        staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.MipLODBias = 0;
        staticSampler.MaxAnisotropy = 1;
        staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        staticSampler.MinLOD = 0.0f;
        staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
        staticSampler.ShaderRegister = 2;    // s2
        staticSampler.RegisterSpace = 0;
        staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // 3) Create Root Signature
        D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
        rsDesc.NumParameters = _countof(params);
        rsDesc.pParameters = params;
        rsDesc.NumStaticSamplers = 1;
        rsDesc.pStaticSamplers = &staticSampler;
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        if (!Create(L"VolumetricCloudRS", rsDesc))
            throw std::runtime_error("RootSignatureManager::Create(\"VolumetricCloudRS\") failed");
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
