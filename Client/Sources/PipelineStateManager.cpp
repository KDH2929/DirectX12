#include "PipelineStateManager.h"
#include "Renderer.h"
#include <stdexcept>
#include <format>
#include <codecvt>


PipelineStateManager::PipelineStateManager(Renderer* renderer_)
    : renderer(renderer_)
    , device(renderer_->GetDevice())
{}

PipelineStateManager::~PipelineStateManager() {
    Cleanup();
}

bool PipelineStateManager::InitializePSOs()
{
    psoMap.clear();         // 캐시 비우기

    // 1. Triangle
    {
        PipelineStateDesc desc = CreateTrianglePSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    // 2. Phong-Lighting
    {
        PipelineStateDesc desc = CreatePhongPSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    // 3. PBR-Lighting
    {
        PipelineStateDesc desc = CreatePbrPSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    // 4. Skybox
    {
        PipelineStateDesc desc = CreateSkyboxPSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    // 5. 노말 디버그용 PSO
    {
        PipelineStateDesc desc = CreateDebugNormalPSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    // 6. OutlinePostEffect PSO
    {
        PipelineStateDesc desc = CreateOutlinePostEffectPSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    // 7. ToneMappingPostEffect PSO
    {
        PipelineStateDesc desc = CreateToneMappingPostEffectPSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    // 8. ShadowMapPass PSO
    {
        PipelineStateDesc desc = CreateShadowMapPassPSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    // 9. VolumetricCloud PSO
    /*
    {
        PipelineStateDesc desc = CreateVolumetricCloudPSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }
    */

    return true;
}

PipelineStateDesc PipelineStateManager::CreateTrianglePSODesc() const {
    // 루트 시그니처
    auto rs = renderer->GetRootSignatureManager()->Get(L"TriangleRS");
    if (!rs)
        throw std::runtime_error("TriangleRS not created");

    // 입력 레이아웃
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    // 쉐이더 Blob
    ComPtr<ID3DBlob> vsBlob = renderer->GetShaderManager()->GetShaderBlob(L"TriangleVS");
    ComPtr<ID3DBlob> psBlob = renderer->GetShaderManager()->GetShaderBlob(L"TrianglePS");

    PipelineStateDesc desc;
    desc.name = L"TrianglePSO";
    desc.rootSignature = rs;
    desc.vsBlob = vsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = inputLayout;

    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // 나머지 상태는 기본값 유지
    return desc;
}

PipelineStateDesc PipelineStateManager::CreatePhongPSODesc() const
{

    // 1) 루트 시그니처 : PhongRS
    auto rs = renderer->GetRootSignatureManager()->Get(L"PhongRS");
    if (!rs)
        throw std::runtime_error("PhongRS not created");


    // 2) 입력 레이아웃 : Position + Normal + TexCoord + Tangent
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // 3) 셰이더 : Phong 전용 VS / PS

    ComPtr<ID3DBlob> vsBlob =
        renderer->GetShaderManager()->GetShaderBlob(L"PhongVS");
    ComPtr<ID3DBlob> psBlob =
        renderer->GetShaderManager()->GetShaderBlob(L"PhongPS");


    // 4) 파이프라인 상태 기본값 구성

    PipelineStateDesc desc;
    desc.name = L"PhongPSO";
    desc.rootSignature = rs;
    desc.vsBlob = vsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = std::move(inputLayout);

    // 필요 시 CullMode 등을 조정
    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // MSAA, RTV/DSV 포맷, SampleMask 등은
    // 프로젝트 공통 기본값 그대로 두거나 여기서 수정
    return desc;
}

PipelineStateDesc PipelineStateManager::CreatePbrPSODesc() const
{
    // 1) RootSignature : PbrRS (bindless)
    auto rs = renderer->GetRootSignatureManager()->Get(L"PbrRS");
    if (!rs)
        throw std::runtime_error("PbrRS not created");

    // 2) InputLayout : Position / Normal / TexCoord / Tangent
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // 3) Shaders : PBR 전용 VS / PS
    ComPtr<ID3DBlob> vsBlob =
        renderer->GetShaderManager()->GetShaderBlob(L"PbrVS");
    ComPtr<ID3DBlob> psBlob =
        renderer->GetShaderManager()->GetShaderBlob(L"PbrPS");

    // 4) 기본 파이프라인 설정
    PipelineStateDesc desc;
    desc.name = L"PbrPSO";
    desc.rootSignature = rs;
    desc.vsBlob = vsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = std::move(inputLayout);

    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // HDR/톤매핑을 고려해 sRGB 백버퍼라면 RTV 포맷도 맞출 것
    desc.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    return desc;
}

PipelineStateDesc PipelineStateManager::CreateSkyboxPSODesc() const
{
    auto rootSig = renderer->GetRootSignatureManager()->Get(L"SkyboxRS");
    if (!rootSig) throw std::runtime_error("SkyboxRS not created");

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    ComPtr<ID3DBlob> vsBlob =
        renderer->GetShaderManager()->GetShaderBlob(L"SkyboxVS");
    ComPtr<ID3DBlob> psBlob =
        renderer->GetShaderManager()->GetShaderBlob(L"SkyboxPS");

    PipelineStateDesc desc;
    desc.name = L"SkyboxPSO";
    desc.rootSignature = rootSig;
    desc.vsBlob = vsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = std::move(inputLayout);

    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;  // draw inside faces

    desc.depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    desc.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    desc.sampleMask = UINT_MAX;
    desc.sampleDesc.Count = 1;

    // HDR/톤매핑을 고려해 sRGB 백버퍼라면 RTV 포맷도 맞출 것
    desc.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    return desc;
}

PipelineStateDesc PipelineStateManager::CreateDebugNormalPSODesc() const
{
    // 1) DebugNormal 전용 루트 시그니처 가져오기
    auto rootSig = renderer->GetRootSignatureManager()->Get(L"DebugNormalRS");
    if (!rootSig)
        throw std::runtime_error("DebugNormalRS not created");


    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
          0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,
          0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,
          0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
          0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // 3) 셰이더 바이트코드
    ComPtr<ID3DBlob> vsBlob = renderer->GetShaderManager()->GetShaderBlob(L"DebugNormalVS");
    ComPtr<ID3DBlob> gsBlob = renderer->GetShaderManager()->GetShaderBlob(L"DebugNormalGS");
    ComPtr<ID3DBlob> psBlob = renderer->GetShaderManager()->GetShaderBlob(L"DebugNormalPS");


    // 4) PSO 기술자 세팅
    PipelineStateDesc desc;
    desc.name = L"DebugNormalPSO";
    desc.rootSignature = rootSig;
    desc.vsBlob = vsBlob;
    desc.gsBlob = gsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = std::move(inputLayout);

    // 5) 래스터라이저: 백페이스 컬링 없애고 와이어나 라인도 잘 보이도록
    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

    // 6) 깊이-스텐실
    desc.depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

    // 8) 샘플링
    desc.sampleMask = UINT_MAX;
    desc.sampleDesc.Count = 1;

    // HDR/톤매핑을 고려해 sRGB 백버퍼라면 RTV 포맷도 맞출 것
    desc.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    return desc;
}

PipelineStateDesc PipelineStateManager::CreateOutlinePostEffectPSODesc() const
{
    auto rootSig = renderer->GetRootSignatureManager()->Get(L"PostProcessRS");
    if (!rootSig)
        throw std::runtime_error("PostProcessRS not created");


    // 풀스크린 삼각형/쿼드용
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
          0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
          24,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    auto vsBlob = renderer->GetShaderManager()->GetShaderBlob(L"OutlinePostEffectVS");
    auto psBlob = renderer->GetShaderManager()->GetShaderBlob(L"OutlinePostEffectPS");

    PipelineStateDesc desc;
    desc.name = L"OutlinePostEffectPSO";
    desc.rootSignature = rootSig;
    desc.vsBlob = vsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = std::move(inputLayout);
    desc.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;


    desc.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.blendDesc.RenderTarget[0].BlendEnable = TRUE;
    desc.blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    desc.blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    desc.blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    desc.blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    desc.blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    desc.blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    // 풀스크린 패스라 DepthStencil은 쓰지 않음
    desc.depthStencilDesc.DepthEnable = FALSE;
    desc.depthStencilDesc.StencilEnable = FALSE;

    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    // 샘플링
    desc.sampleMask = UINT_MAX;
    desc.sampleDesc.Count = 1;

    return desc;
}

PipelineStateDesc PipelineStateManager::CreateToneMappingPostEffectPSODesc() const
{
    // 공통 PostProcess Root Signature 사용
    auto rootSig = renderer->GetRootSignatureManager()->Get(L"PostProcessRS");
    if (!rootSig)
        throw std::runtime_error("PostProcessRS not created");

    // 풀스크린 삼각형/쿼드용 입력 레이아웃
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
          0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0,
          24,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // 톤매핑용 셰이더
    auto vsBlob = renderer->GetShaderManager()->GetShaderBlob(L"ToneMappingPostEffectVS");
    auto psBlob = renderer->GetShaderManager()->GetShaderBlob(L"ToneMappingPostEffectPS");

    PipelineStateDesc desc;
    desc.name = L"ToneMappingPostEffectPSO";
    desc.rootSignature = rootSig;
    desc.vsBlob = vsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = std::move(inputLayout);
    desc.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    desc.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    desc.depthStencilDesc.DepthEnable = FALSE;
    desc.depthStencilDesc.StencilEnable = FALSE;

    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    desc.sampleMask = UINT_MAX;
    desc.sampleDesc.Count = 1;

    return desc;
}

PipelineStateDesc PipelineStateManager::CreateShadowMapPassPSODesc() const
{
    auto rootSig = renderer->GetRootSignatureManager()->Get(L"ShadowMapPassRS");
    if (!rootSig)
        throw std::runtime_error("ShadowMapPassRS not created");

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
          0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    auto vsBlob = renderer->GetShaderManager()->GetShaderBlob(L"ShadowMapPassVS");
    auto psBlob = renderer->GetShaderManager()->GetShaderBlob(L"ShadowMapPassPS");

    PipelineStateDesc desc;
    desc.name = L"ShadowMapPassPSO";
    desc.rootSignature = rootSig;
    desc.vsBlob = vsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = std::move(inputLayout);
    desc.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // 깊이만 기록, RTV 없음
    desc.rtvFormats[0] = DXGI_FORMAT_UNKNOWN;
    desc.numRenderTargets = 0;

    // Depth 사용
    desc.depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.depthStencilDesc.DepthEnable = TRUE;
    desc.depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    desc.depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    desc.depthStencilDesc.StencilEnable = FALSE;

    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.sampleMask = UINT_MAX;
    desc.sampleDesc.Count = 1;

    return desc;
}

PipelineStateDesc PipelineStateManager::CreateVolumetricCloudPSODesc() const
{

    auto rootSig = renderer->GetRootSignatureManager()->Get(L"VolumetricCloudRS");
    if (!rootSig)
        throw std::runtime_error("VolumetricCloudRS not created");

    // No vertex buffer layout (we use SV_VertexID / point list)
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;


    auto vsBlob = renderer->GetShaderManager()->GetShaderBlob(L"VolumetricCloudVS");
    auto gsBlob = renderer->GetShaderManager()->GetShaderBlob(L"VolumetricCloudGS");
    auto psBlob = renderer->GetShaderManager()->GetShaderBlob(L"VolumetricCloudPS");


    PipelineStateDesc desc;
    desc.name = L"VolumetricCloudPSO";
    desc.rootSignature = rootSig;
    desc.vsBlob = vsBlob;
    desc.gsBlob = gsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = std::move(inputLayout);


    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

    desc.depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.depthStencilDesc.DepthEnable = TRUE;
    desc.depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    desc.depthStencilDesc.StencilEnable = FALSE;

    desc.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    auto& rt0 = desc.blendDesc.RenderTarget[0];
    rt0.BlendEnable = TRUE;
    rt0.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    rt0.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    rt0.BlendOp = D3D12_BLEND_OP_ADD;
    rt0.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt0.DestBlendAlpha = D3D12_BLEND_ZERO;
    rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rt0.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    desc.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

    desc.sampleMask = UINT_MAX;
    desc.sampleDesc.Count = 1;

    return desc;
}

ID3D12PipelineState* PipelineStateManager::GetOrCreate(
    const PipelineStateDesc& desc)
{
    auto it = psoMap.find(desc.name);
    if (it != psoMap.end())
        return it->second.Get();

    if (!CreatePSO(desc))
        return nullptr;

    return psoMap[desc.name].Get();
}

bool PipelineStateManager::CreatePSO(
    const PipelineStateDesc& desc)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = desc.rootSignature.Get();
    psoDesc.VS = { desc.vsBlob->GetBufferPointer(), desc.vsBlob->GetBufferSize() };

    if (desc.gsBlob)
        psoDesc.GS = { desc.gsBlob->GetBufferPointer(), desc.gsBlob->GetBufferSize() };

    psoDesc.PS = { desc.psBlob->GetBufferPointer(), desc.psBlob->GetBufferSize() };
    psoDesc.InputLayout = { desc.inputLayout.data(), static_cast<UINT>(desc.inputLayout.size()) };
    psoDesc.RasterizerState = desc.rasterizerDesc;
    psoDesc.BlendState = desc.blendDesc;
    psoDesc.DepthStencilState = desc.depthStencilDesc;
    psoDesc.SampleMask = desc.sampleMask;
    psoDesc.PrimitiveTopologyType = desc.topologyType;
    psoDesc.NumRenderTargets = desc.numRenderTargets;
    for (UINT i = 0; i < psoDesc.NumRenderTargets; ++i)
        psoDesc.RTVFormats[i] = desc.rtvFormats[i];
    psoDesc.DSVFormat = desc.dsvFormat;
    psoDesc.SampleDesc = desc.sampleDesc;

    psoDesc.NodeMask = 1;                          // GPU 0 사용
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    ComPtr<ID3D12PipelineState> pipelineState;
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    if (FAILED(hr)) {
        throw std::runtime_error("CreateGraphicsPipelineState failed");
    }
    psoMap[desc.name] = pipelineState;

    return true;
}

ID3D12PipelineState* PipelineStateManager::Get(
    const std::wstring& name) const
{
    auto it = psoMap.find(name);
    return it != psoMap.end() ? it->second.Get() : nullptr;
}

void PipelineStateManager::Cleanup() {
    psoMap.clear();
    renderer = nullptr;
    device = nullptr;
}
