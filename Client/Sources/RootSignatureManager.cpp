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

    // 1) TriangleRS 생성
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


    // 2) 퐁 조명용 PhongRS 생성
    {
        // b0~b3: CBVs for MVP, Lighting, Material, Global
        D3D12_ROOT_PARAMETER params[6] = {};          //-- 초기화
        for (int i = 0; i < 4; ++i) {
            params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            params[i].Descriptor.ShaderRegister = i;      // b0, b1, b2, b3
            params[i].Descriptor.RegisterSpace = 0;
            params[i].ShaderVisibility =
                (i == 0 ? D3D12_SHADER_VISIBILITY_VERTEX
                    : D3D12_SHADER_VISIBILITY_PIXEL);
        }

        // t0, t1 -- 알베도 + 노멀  → 한 테이블에 2 개 SRV
        D3D12_DESCRIPTOR_RANGE srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 2;        // 1  →  2
        srvRange.BaseShaderRegister = 0;      // t0
        srvRange.RegisterSpace = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // s0  (샘플러 테이블은 그대로)
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

    // 3) PBR용 루트 시그니처 PbrRS 생성
    {
        // b0~b4: CBV (b0=MVP, b1=Lighting, b2=Material, b3=Global, b4=ShadowViewProj)
        D3D12_ROOT_PARAMETER params[11] = {};
        for (UINT i = 0; i < 5; ++i)
        {
            params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            params[i].Descriptor.ShaderRegister = i;  // b0, b1, b2, b3
            params[i].Descriptor.RegisterSpace = 0;
            params[i].ShaderVisibility = (i == 0)
                ? D3D12_SHADER_VISIBILITY_VERTEX
                : D3D12_SHADER_VISIBILITY_PIXEL;
        }

        // Material(4) Descriptor Table → root 5
        D3D12_DESCRIPTOR_RANGE matRange{};
        matRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        matRange.NumDescriptors = 4;   // t0~t3
        matRange.BaseShaderRegister = 0;
        matRange.RegisterSpace = 0;
        matRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[5].DescriptorTable.NumDescriptorRanges = 1;
        params[5].DescriptorTable.pDescriptorRanges = &matRange;
        params[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // IrradianceCube → root 6 (t4)
        D3D12_DESCRIPTOR_RANGE irrRange = matRange;
        irrRange.NumDescriptors = 1;
        irrRange.BaseShaderRegister = 4;  // t4

        params[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[6].DescriptorTable.NumDescriptorRanges = 1;
        params[6].DescriptorTable.pDescriptorRanges = &irrRange;
        params[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // PrefilteredCube → root 7 (t5)
        D3D12_DESCRIPTOR_RANGE prefRange = matRange;
        prefRange.NumDescriptors = 1;
        prefRange.BaseShaderRegister = 5;  // t5

        params[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[7].DescriptorTable.NumDescriptorRanges = 1;
        params[7].DescriptorTable.pDescriptorRanges = &prefRange;
        params[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // BRDF LUT → root 8 (t6)
        D3D12_DESCRIPTOR_RANGE lutRange = matRange;
        lutRange.NumDescriptors = 1;
        lutRange.BaseShaderRegister = 6;  // t6

        params[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[8].DescriptorTable.NumDescriptorRanges = 1;
        params[8].DescriptorTable.pDescriptorRanges = &lutRange;
        params[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

       

        // Sampler 테이블 → root 9 (s0~s1)
        D3D12_DESCRIPTOR_RANGE sampRange{};
        sampRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        sampRange.NumDescriptors = 2;      // s0, s1
        sampRange.BaseShaderRegister = 0;     // s0 시작
        sampRange.RegisterSpace = 0;
        sampRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[9].DescriptorTable.NumDescriptorRanges = 1;
        params[9].DescriptorTable.pDescriptorRanges = &sampRange;
        params[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // ShadowMap → root 10
        D3D12_DESCRIPTOR_RANGE shadowRange{};
        shadowRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        shadowRange.NumDescriptors = MAX_SHADOW_DSV_COUNT;       // t7 ~ t7+N-1
        shadowRange.BaseShaderRegister = 7;                   // t7
        shadowRange.RegisterSpace = 0;
        shadowRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[10].DescriptorTable.NumDescriptorRanges = 1;
        params[10].DescriptorTable.pDescriptorRanges = &shadowRange;
        params[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

   

        D3D12_STATIC_SAMPLER_DESC shadowMapSamplerDesc{};
        shadowMapSamplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        shadowMapSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        shadowMapSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        shadowMapSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        shadowMapSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        shadowMapSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        shadowMapSamplerDesc.MinLOD = 0;
        shadowMapSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        shadowMapSamplerDesc.ShaderRegister = 2;
        shadowMapSamplerDesc.RegisterSpace = 0;
        shadowMapSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        

        // 루트 시그니처 Desc 생성
        D3D12_ROOT_SIGNATURE_DESC desc{};
        desc.NumParameters = _countof(params);
        desc.pParameters = params;
        desc.NumStaticSamplers = 1;
        desc.pStaticSamplers = &shadowMapSamplerDesc;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // 8) 생성 호출
        if (!Create(L"PbrRS", desc))
            throw std::runtime_error("RootSignatureManager::Create(\"PbrRS\") failed");
    }


    // 4) Skybox(큐브맵)용 루트 시그니처
    {
        // b0 : CB_MVP (world-view-proj)
        // t0 : 큐브맵 SRV
        // s0 : 샘플러
        D3D12_ROOT_PARAMETER params[3] = {};

        // CBV b0
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0;
        params[0].Descriptor.RegisterSpace = 0;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // SRV 테이블 t0
        D3D12_DESCRIPTOR_RANGE srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 1;   // 큐브맵 한 개
        srvRange.BaseShaderRegister = 0;   // t0
        srvRange.RegisterSpace = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[1].DescriptorTable.NumDescriptorRanges = 1;
        params[1].DescriptorTable.pDescriptorRanges = &srvRange;
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // 샘플러 테이블 s0
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

        // 루트 시그니처 설명
        D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
        rsDesc.NumParameters = _countof(params);
        rsDesc.pParameters = params;
        rsDesc.NumStaticSamplers = 0;
        rsDesc.pStaticSamplers = nullptr;
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // 생성 호출 ("SkyboxRS"로 식별)
        if (!Create(L"SkyboxRS", rsDesc))
            throw std::runtime_error("RootSignatureManager::Create(\"SkyboxRS\") failed");
    }


    // DebugNormal RS
    {
        // 1) 파라미터 배열: CBV b0 하나
        D3D12_ROOT_PARAMETER debugParams[1] = {};
        debugParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        debugParams[0].Descriptor.ShaderRegister = 0; // b0
        debugParams[0].Descriptor.RegisterSpace = 0;
        debugParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // VS/GS/PS 전부 사용

        // 2) 루트 시그니처 Desc
        D3D12_ROOT_SIGNATURE_DESC debugRSDesc = {};
        debugRSDesc.NumParameters = 1;
        debugRSDesc.pParameters = debugParams;
        debugRSDesc.NumStaticSamplers = 0;
        debugRSDesc.pStaticSamplers = nullptr;
        debugRSDesc.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // 3) 생성
        Create(L"DebugNormalRS", debugRSDesc);
    }


    // PostProcess RS
    {
        //  b0 : 파라미터 CBV ( PostProcess 옵션들 )
        //  t0 : 입력 텍스처 (SceneColor, 또는 별도 SRV 슬롯)
        //  s0 : 정적샘플러 (linear clamp)
        D3D12_ROOT_PARAMETER postParams[2] = {};

        // b0 → CBV
        postParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        postParams[0].Descriptor.ShaderRegister = 0;      // b0
        postParams[0].Descriptor.RegisterSpace = 0;
        postParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // t0 → SRV 테이블
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

        // 생성
        Create(L"PostProcessRS", postDesc);
    }

    
    // ShadowMapPass RS :  깊이값만 기록함
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

bool RootSignatureManager::Create(const std::wstring& name,
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& descIn)
{
    if (signatureMap.contains(name))
        return true;

    // 1) 하드웨어가 v1.1 (bindless) 루트 시그니처를 지원하는지 확인
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{ D3D_ROOT_SIGNATURE_VERSION_1_1 };
    bool supportV11 = SUCCEEDED(device->CheckFeatureSupport(
        D3D12_FEATURE_ROOT_SIGNATURE,
        &featureData,
        sizeof(featureData)))
        && featureData.HighestVersion >= D3D_ROOT_SIGNATURE_VERSION_1_1;

    // 2) desc 복사 후, 지원 안 하면 직접 인덱싱 플래그 제거
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = descIn;
    if (!supportV11) {
        OutputDebugStringA("Bindless not supported; stripping direct-index flags\n");
        // v1.0 경로로 강제 폴백
        desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
        // Desc_1_0는 파라미터·flags만 사용; 직접 인덱싱 플래그 제거
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

    // 4) CreateRootSignature 호출
    //    v1.1 지원 시 ID3D12Device1, 아니면 ID3D12Device
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
