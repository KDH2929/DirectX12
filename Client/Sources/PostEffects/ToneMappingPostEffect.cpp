#include "ToneMappingPostEffect.h"
#include "Renderer.h"
#include "DescriptorHeapManager.h"
#include "PipelineStateManager.h"
#include "RootSignatureManager.h"
#include "Mesh.h"
#include <imgui.h>

using namespace DirectX;

void ToneMappingPostEffect::Initialize(Renderer* renderer)
{
    // 1) ��� ���� ���� (256B aligned Upload)
    ID3D12Device* device = renderer->GetDevice();
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(256);

    if (FAILED(device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&toneMapConstantBuffer))))
    {
        throw std::runtime_error("Failed to create tone map constant buffer");
    }

    // Map �� ���� ����
    toneMapConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedToneMapOptions));

    // 2) Ǯ��ũ�� ���� ����
    quadMesh = Mesh::CreateQuad(renderer);
    if (!quadMesh)
        throw std::runtime_error("Failed to create quad mesh for ToneMappingPostEffect");
}

void ToneMappingPostEffect::Update(float deltaTime)
{
    ImGui::Begin("ToneMappingPostEffect");

    ImGui::SliderFloat("Exposure", &exposureValue, 0.1f, 5.0f);
    ImGui::SliderFloat("Gamma", &gammaValue, 1.0f, 3.0f);

    ImGui::End();
}

void ToneMappingPostEffect::Render(Renderer* renderer)
{
    // ��� ���ۿ� ������ ����
    CB_ToneMapping opts{};
    opts.Exposure = exposureValue;
    opts.Gamma = gammaValue;

    memcpy(mappedToneMapOptions, &opts, sizeof(opts));

    // 1) Ŀ�ǵ� ����Ʈ, �� �Ŵ���
    auto commandList = renderer->GetDirectCommandList();
    auto descriptorHeapManager = renderer->GetDescriptorHeapManager();

    // 2) CBV_SRV_UAV �� ���ε� (t0: scene SRV, b0: tone map CBV)
    ID3D12DescriptorHeap* heaps[] = {
        descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // 3) ��Ʈ �ñ״�ó & PSO ���ε�
    commandList->SetGraphicsRootSignature(
        renderer->GetRootSignatureManager()->Get(L"PostProcessRS"));
    commandList->SetPipelineState(
        renderer->GetPSOManager()->Get(L"ToneMappingPostEffectPSO"));

    // 4) ��� ���� (b0) ���ε�
    commandList->SetGraphicsRootConstantBufferView(
        0, toneMapConstantBuffer->GetGPUVirtualAddress());

    // 5) SRV ���̺� (t0) ���ε�
    auto sceneColorSrvHandle = renderer->GetSceneColorSrvHandle();
    commandList->SetGraphicsRootDescriptorTable(1, sceneColorSrvHandle);

    // 6) Ǯ��ũ�� ���� Draw
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &quadMesh->GetVertexBufferView());
    commandList->IASetIndexBuffer(&quadMesh->GetIndexBufferView());
    commandList->DrawIndexedInstanced(quadMesh->GetIndexCount(), 1, 0, 0, 0);
}