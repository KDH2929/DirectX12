#include "Skybox.h"
#include "Renderer.h"
#include "FrameResource/FrameResource.h"
#include <directx/d3dx12.h>
#include <cassert>
using namespace DirectX;

Skybox::Skybox(std::shared_ptr<Texture> texture)
    : cubeMapTexture(std::move(texture))
{
    position = { 0.0f, 0.0f, 0.0f };
    scale = { 100.0f, 100.0f, 100.0f };
    rotation = XMQuaternionIdentity();
    UpdateWorldMatrix();
}

bool Skybox::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
        return false;

    cubeMesh = Mesh::CreateCube(renderer);
    if (!cubeMesh)
        return false;

    rootSignature = renderer->GetRootSignatureManager()->Get(L"SkyboxRS");
    pipelineState = renderer->GetPSOManager()->Get(L"SkyboxPSO");
    return (rootSignature.Get() && pipelineState.Get());
}

void Skybox::Update(float /*deltaTime*/, Renderer* renderer, UINT objectIndex)
{
    UpdateWorldMatrix();
    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    assert(frameResource && frameResource->cbMVP && "FrameResource or cbMVP is null");

    CB_MVP cb{};
    XMStoreFloat4x4(&cb.model, XMMatrixTranspose(worldMatrix));
    XMMATRIX view = renderer->GetCamera()->GetViewMatrix();
    view.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMStoreFloat4x4(&cb.view, XMMatrixTranspose(view));
    XMStoreFloat4x4(&cb.projection, XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
    XMStoreFloat4x4(&cb.modelInvTranspose, XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));

    frameResource->cbMVP->CopyData(objectIndex, cb);
}

void Skybox::Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex)
{
    // Descriptor Heaps 바인딩 (CBV_SRV_UAV + SAMPLER)
    auto* heapManager = renderer->GetDescriptorHeapManager();
    ID3D12DescriptorHeap* descriptorHeaps[] = {
        heapManager->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        heapManager->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // RS & PSO 바인딩
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pipelineState.Get());

    // b0: 모델-뷰-투영 상수 버퍼
    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    commandList->SetGraphicsRootConstantBufferView(
        0, frameResource->cbMVP->GetGPUVirtualAddress(objectIndex)
    );

    // t0: 큐브맵 SRV
    commandList->SetGraphicsRootDescriptorTable(
        1, cubeMapTexture->GetGpuHandle()
    );

    // s0: 샘플러
    commandList->SetGraphicsRootDescriptorTable(
        2, heapManager->GetLinearWrapSamplerGpuHandle()
    );

    // IA 설정 및 드로우 호출
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &cubeMesh->GetVertexBufferView());
    commandList->IASetIndexBuffer(&cubeMesh->GetIndexBufferView());
    commandList->DrawIndexedInstanced(cubeMesh->GetIndexCount(), 1, 0, 0, 0);
}