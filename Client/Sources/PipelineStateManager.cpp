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
    psoMap.clear();         // ĳ�� ����

    // 1. Triangle�� PSO
    {
        PipelineStateDesc desc = CreateTrianglePSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    // 2. Phong-Lighting�� PSO
    {
        PipelineStateDesc desc = CreatePhongPSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    // 3. PBR-Lighting ����
    {
        PipelineStateDesc desc = CreatePbrPSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    // 4. Skybox ����
    {
        PipelineStateDesc desc = CreateSkyboxPSODesc();
        if (GetOrCreate(desc) == nullptr)
            return false;
    }

    return true;
}

PipelineStateDesc PipelineStateManager::CreateTrianglePSODesc() const {
    // ��Ʈ �ñ״�ó
    auto rs = renderer->GetRootSignatureManager()->Get(L"TriangleRS");
    if (!rs)
        throw std::runtime_error("TriangleRS not created");

    // �Է� ���̾ƿ�
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    // ���̴� Blob
    ComPtr<ID3DBlob> vsBlob = renderer->GetShaderManager()->GetShaderBlob(L"TriangleVertexShader");
    ComPtr<ID3DBlob> psBlob = renderer->GetShaderManager()->GetShaderBlob(L"TrianglePixelShader");

    PipelineStateDesc desc;
    desc.name = L"TrianglePSO";
    desc.rootSignature = rs;
    desc.vsBlob = vsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = inputLayout;

    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // ������ ���´� �⺻�� ����
    return desc;
}

PipelineStateDesc PipelineStateManager::CreatePhongPSODesc() const
{

    // 1) ��Ʈ �ñ״�ó : PhongRS
    auto rs = renderer->GetRootSignatureManager()->Get(L"PhongRS");
    if (!rs)
        throw std::runtime_error("PhongRS not created");


    // 2) �Է� ���̾ƿ� : Position + Normal + TexCoord + Tangent
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

    // 3) ���̴� : Phong ���� VS / PS

    ComPtr<ID3DBlob> vsBlob =
        renderer->GetShaderManager()->GetShaderBlob(L"PhongVertexShader");
    ComPtr<ID3DBlob> psBlob =
        renderer->GetShaderManager()->GetShaderBlob(L"PhongPixelShader");


    // 4) ���������� ���� �⺻�� ����

    PipelineStateDesc desc;
    desc.name = L"PhongPSO";
    desc.rootSignature = rs;
    desc.vsBlob = vsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = std::move(inputLayout);

    // �ʿ� �� CullMode ���� ����
    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // MSAA, RTV/DSV ����, SampleMask ����
    // ������Ʈ ���� �⺻�� �״�� �ΰų� ���⼭ ����
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

    // 3) Shaders : PBR ���� VS / PS
    ComPtr<ID3DBlob> vsBlob =
        renderer->GetShaderManager()->GetShaderBlob(L"PbrVertexShader");
    ComPtr<ID3DBlob> psBlob =
        renderer->GetShaderManager()->GetShaderBlob(L"PbrPixelShader");

    // 4) �⺻ ���������� ����
    PipelineStateDesc desc;
    desc.name = L"PbrPSO";
    desc.rootSignature = rs;
    desc.vsBlob = vsBlob;
    desc.psBlob = psBlob;
    desc.inputLayout = std::move(inputLayout);

    desc.rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // HDR/������� ����� sRGB ����۶�� RTV ���˵� ���� ��
    // ��) desc.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

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
        renderer->GetShaderManager()->GetShaderBlob(L"SkyboxVertexShader");
    ComPtr<ID3DBlob> psBlob =
        renderer->GetShaderManager()->GetShaderBlob(L"SkyboxPixelShader");

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

    psoDesc.NodeMask = 1;                          // GPU 0 ���
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
