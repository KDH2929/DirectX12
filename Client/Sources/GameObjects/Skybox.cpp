#include "Skybox.h"
#include "Renderer.h"
#include <directx/d3dx12.h>

using namespace DirectX;


Skybox::Skybox(std::shared_ptr<Texture> tex)
    : cubeMapTexture(std::move(tex))
{
    position = { 0,0,0 };
    scale = { 100,100,100 };
    rotation = XMQuaternionIdentity();
    UpdateWorldMatrix();
}

bool Skybox::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
        return false;

    // 1) 메시 생성
    cubeMesh = Mesh::CreateCube(renderer);
    if (!cubeMesh)
        return false;

    // 2) RS/PSO 참조만
    rootSignature = renderer->GetRootSignatureManager()->Get(L"SkyboxRS");
    pipelineState = renderer->GetPSOManager()->Get(L"SkyboxPSO");
    return (rootSignature && pipelineState);
}

void Skybox::Update(float /*deltaTime*/)
{
}

void Skybox::Render(Renderer* renderer)
{
    // 1) Direct command list
    ID3D12GraphicsCommandList* directCommandList =
        renderer->GetDirectCommandList();

    // 2) Descriptor Heaps
    auto* heapManager = renderer->GetDescriptorHeapManager();
    ID3D12DescriptorHeap* descriptorHeaps[] = {
        heapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        heapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    directCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // 3) RS & PSO
    directCommandList->SetGraphicsRootSignature(
        renderer->GetRootSignatureManager()->Get(L"SkyboxRS"));
    directCommandList->SetPipelineState(
        renderer->GetPSOManager()->Get(L"SkyboxPSO"));

    // 4) MVP CBV (b0)
    {
        CB_MVP cb{};
        XMStoreFloat4x4(&cb.model, XMMatrixTranspose(worldMatrix));
        XMMATRIX view = renderer->GetCamera()->GetViewMatrix();
        view.r[3] = XMVectorSet(0, 0, 0, 1);
        XMStoreFloat4x4(&cb.view, XMMatrixTranspose(view));
        XMStoreFloat4x4(&cb.projection, XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
        XMStoreFloat4x4(&cb.modelInvTranspose, XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));
        memcpy(mappedMVPData, &cb, sizeof(cb));
        directCommandList->SetGraphicsRootConstantBufferView(
            0, constantMVPBuffer->GetGPUVirtualAddress());
    }

    // 5) SRV t0
    {
        auto srvHeap = descriptorHeaps[0];
        UINT stride = renderer->GetDescriptorHeapManager()->GetStride(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        auto handle = srvHeap->GetGPUDescriptorHandleForHeapStart();
        handle.ptr += SIZE_T(cubeMapTexture->GetDescriptorIndex()) * stride;
        directCommandList->SetGraphicsRootDescriptorTable(1, handle);
    }


    // 6) Sampler s0
    directCommandList->SetGraphicsRootDescriptorTable(
        2, descriptorHeaps[1]->GetGPUDescriptorHandleForHeapStart());

    // 7) IA & Draw
    directCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    directCommandList->IASetVertexBuffers(0, 1, &cubeMesh->GetVertexBufferView());
    directCommandList->IASetIndexBuffer(&cubeMesh->GetIndexBufferView());
    directCommandList->DrawIndexedInstanced(cubeMesh->GetIndexCount(), 1, 0, 0, 0);
}