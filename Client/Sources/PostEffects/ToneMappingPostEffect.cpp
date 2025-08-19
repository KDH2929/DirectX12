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
    // 풀스크린 쿼드 생성
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

    // CBV_SRV_UAV 힙 바인딩 (t0: scene SRV, b0: tone map CBV)
    ID3D12DescriptorHeap* heaps[] = {
        descriptorHeapManager->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // 루트 시그니처 & PSO 바인딩
    commandList->SetGraphicsRootSignature(
        renderer->GetRootSignatureManager()->Get(L"PostProcessRS"));
    commandList->SetPipelineState(
        renderer->GetPSOManager()->Get(L"ToneMappingPostEffectPSO"));

    // 상수 버퍼 (b0) 바인딩
    commandList->SetGraphicsRootConstantBufferView(0, frameResource->cbToneMapping->GetGPUVirtualAddress(0));

    // SRV 테이블 (t0) 바인딩
    commandList->SetGraphicsRootDescriptorTable(1, frameResource->sceneColorSrv.gpuHandle);

    // 풀스크린 쿼드 Draw
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &quadMesh->GetVertexBufferView());
    commandList->IASetIndexBuffer(&quadMesh->GetIndexBufferView());
    commandList->DrawIndexedInstanced(quadMesh->GetIndexCount(), 1, 0, 0, 0);
}