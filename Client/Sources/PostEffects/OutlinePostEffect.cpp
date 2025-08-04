#include "OutlinePostEffect.h"
#include "Renderer.h"
#include "DescriptorHeapManager.h"
#include "PipelineStateManager.h"
#include "RootSignatureManager.h"
#include "Mesh.h"
#include <imgui.h>

using namespace DirectX;

void OutlinePostEffect::Initialize(Renderer* renderer)
{
    // 풀스크린 쿼드 생성
    quadMesh = Mesh::CreateQuad(renderer);
    if (!quadMesh)
        throw std::runtime_error("Failed to create quad mesh for OutlinePostEffect");
}

void OutlinePostEffect::Update(float deltaTime, Renderer* renderer)
{
    ImGui::Begin("OutlinePostEffect");

    ImGui::SliderFloat("Threshold", &thresholdValue, 0.0f, 20.0f);
    ImGui::SliderFloat("Thickness", &thicknessValue, 0.5f, 5.0f);
    ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&outlineColor));
    ImGui::SliderFloat("Mix Factor", &mixFactor, 0.0f, 1.0f);

    ImGui::End();


    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    
    CB_OutlineOptions outlineOptions{};
    float viewportWidth = static_cast<float>(renderer->GetViewportWidth());
    float viewportHeight = static_cast<float>(renderer->GetViewportHeight());
    outlineOptions.TexelSize = { 1.0f / viewportWidth,  1.0f / viewportHeight };
    outlineOptions.EdgeThreshold = thresholdValue;
    outlineOptions.EdgeThickness = thicknessValue;
    outlineOptions.OutlineColor = outlineColor;
    outlineOptions.MixFactor = mixFactor;
    
    frameResource->cbOutline->CopyData(0, outlineOptions);

}

void OutlinePostEffect::Render(Renderer* renderer)
{
    FrameResource* frameResource = renderer->GetCurrentFrameResource();

    auto commandList = frameResource->commandList.Get();
    auto descriptorHeapManager = renderer->GetDescriptorHeapManager();

    // CBV_SRV_UAV 힙 바인딩 (t0: scene SRV, b0: outline CBV)
    ID3D12DescriptorHeap* heaps[] = {
        descriptorHeapManager->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // 루트 시그니처 & PSO 바인딩
    commandList->SetGraphicsRootSignature(
       renderer->GetRootSignatureManager()->Get(L"PostProcessRS"));
    commandList->SetPipelineState(
        renderer->GetPSOManager()->Get(L"OutlinePostEffectPSO"));

    // 상수 버퍼 (b0) 바인딩
    commandList->SetGraphicsRootConstantBufferView(0, frameResource->cbOutline->GetGPUVirtualAddress(0));

    // SRV 테이블 (t0) 바인딩
    commandList->SetGraphicsRootDescriptorTable(1, frameResource->sceneColorSrv.gpuHandle);

    // 풀스크린 쿼드 Draw
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &quadMesh->GetVertexBufferView());
    commandList->IASetIndexBuffer(&quadMesh->GetIndexBufferView());
    commandList->DrawIndexedInstanced(quadMesh->GetIndexCount(), 1, 0, 0, 0);
}