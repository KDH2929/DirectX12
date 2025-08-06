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
    // Ǯ��ũ�� ���� ����
    quadMesh = Mesh::CreateQuad(renderer);
    if (!quadMesh)
        throw std::runtime_error("Failed to create quad mesh for ToneMappingPostEffect");
}

void ToneMappingPostEffect::Update(float deltaTime, Renderer* renderer)
{
    ImGui::Begin("ToneMappingPostEffect");

    ImGui::SliderFloat("Exposure", &exposureValue, 0.1f, 5.0f);
    ImGui::SliderFloat("Gamma", &gammaValue, 1.0f, 3.0f);

    ImGui::End();

    FrameResource* frameResource = renderer->GetCurrentFrameResource();

    CB_ToneMapping toneMappingOptions{};
    toneMappingOptions.Exposure = exposureValue;
    toneMappingOptions.Gamma = gammaValue;
    frameResource->cbToneMapping->CopyData(0, toneMappingOptions);
}

void ToneMappingPostEffect::Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer)
{
    FrameResource* frameResource = renderer->GetCurrentFrameResource();

    auto descriptorHeapManager = renderer->GetDescriptorHeapManager();

    // CBV_SRV_UAV �� ���ε� (t0: scene SRV, b0: tone map CBV)
    ID3D12DescriptorHeap* heaps[] = {
        descriptorHeapManager->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // ��Ʈ �ñ״�ó & PSO ���ε�
    commandList->SetGraphicsRootSignature(
        renderer->GetRootSignatureManager()->Get(L"PostProcessRS"));
    commandList->SetPipelineState(
        renderer->GetPSOManager()->Get(L"ToneMappingPostEffectPSO"));

    // ��� ���� (b0) ���ε�
    commandList->SetGraphicsRootConstantBufferView(0, frameResource->cbToneMapping->GetGPUVirtualAddress(0));

    // SRV ���̺� (t0) ���ε�
    commandList->SetGraphicsRootDescriptorTable(1, frameResource->sceneColorSrv.gpuHandle);

    // Ǯ��ũ�� ���� Draw
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &quadMesh->GetVertexBufferView());
    commandList->IASetIndexBuffer(&quadMesh->GetIndexBufferView());
    commandList->DrawIndexedInstanced(quadMesh->GetIndexCount(), 1, 0, 0, 0);
}