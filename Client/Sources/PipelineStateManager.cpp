#include "PipelineStateManager.h"
#include "Renderer.h"
#include <stdexcept>

PipelineStateManager::PipelineStateManager(Renderer* renderer_)
    : renderer(renderer_)
    , device(renderer_->GetDevice())
{}

PipelineStateManager::~PipelineStateManager() {
    Cleanup();
}

bool PipelineStateManager::InitializePSOs() {
    psoMap.clear();
    // 삼각형 전용 PSO 생성
    auto triDesc = CreateTrianglePSODesc();
    return GetOrCreate(triDesc) != nullptr;
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

    // 나머지 상태는 기본값 유지
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

    ComPtr<ID3D12PipelineState> pso;
    if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)))) {
        return false;
    }

    psoMap[desc.name] = pso;
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
