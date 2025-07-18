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
    // 1) ��� ���� ���� (256B aligned Upload)
    ID3D12Device* device = renderer->GetDevice();
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC   bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(256);

    if (FAILED(device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&outlineConstantBuffer))))
    {
        throw std::runtime_error("Failed to create outline constant buffer");
    }

    // Map �� ���� ����
    outlineConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedOutlineOptions));

    // 2) Ǯ��ũ�� ���� ����
    quadMesh = Mesh::CreateQuad(renderer);
    if (!quadMesh)
        throw std::runtime_error("Failed to create quad mesh for OutlinePostEffect");
}

void OutlinePostEffect::Update(float deltaTime)
{
    ImGui::Begin("OutlinePostEffect");

    ImGui::SliderFloat("Threshold", &thresholdValue, 0.0f, 20.0f);
    ImGui::SliderFloat("Thickness", &thicknessValue, 0.5f, 5.0f);
    ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&outlineColor));
    ImGui::SliderFloat("Mix Factor", &mixFactor, 0.0f, 1.0f);

    ImGui::End();

}

void OutlinePostEffect::Render(Renderer* renderer)
{

    // ��� ���ۿ� ������ ����
    CB_OutlineOptions options{};
    float width = static_cast<float>(renderer->GetViewportWidth());
    float height = static_cast<float>(renderer->GetViewportHeight());
    options.TexelSize = { 1.0f / width, 1.0f / height };
    options.EdgeThreshold = thresholdValue;
    options.EdgeThickness = thicknessValue;
    options.OutlineColor = outlineColor;
    options.MixFactor = mixFactor;

    memcpy(mappedOutlineOptions, &options, sizeof(options));

    // 1) Ŀ�ǵ� ����Ʈ, �� �Ŵ���
    auto commandList = renderer->GetDirectCommandList();
    auto descriptorHeapManager = renderer->GetDescriptorHeapManager();

    // 2) CBV_SRV_UAV �� ���ε� (t0: scene SRV, b0: outline CBV)
    ID3D12DescriptorHeap* heaps[] = {
        descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // 3) ��Ʈ �ñ״�ó & PSO ���ε�
    commandList->SetGraphicsRootSignature(
       renderer->GetRootSignatureManager()->Get(L"OutlinePostEffectRS"));
    commandList->SetPipelineState(
        renderer->GetPSOManager()->Get(L"OutlinePostEffectPSO"));

    // 4) ��� ���� (b0) ���ε�
    commandList->SetGraphicsRootConstantBufferView(
        0, outlineConstantBuffer->GetGPUVirtualAddress());

    // 5) SRV ���̺� (t0) ���ε�
    auto sceneColorSrvHandle = renderer->GetSceneColorSrvHandle();
    commandList->SetGraphicsRootDescriptorTable(1, sceneColorSrvHandle);

    // 6) Ǯ��ũ�� ���� Draw
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &quadMesh->GetVertexBufferView());
    commandList->IASetIndexBuffer(&quadMesh->GetIndexBufferView());
    commandList->DrawIndexedInstanced(quadMesh->GetIndexCount(), 1, 0, 0, 0);
}