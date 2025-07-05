#include "Flight.h"
#include "Renderer.h"
#include "Mesh.h"
#include "TextureManager.h"
#include "Material.h"
#include <directx/d3dx12.h>
#include <algorithm>

using namespace DirectX;

Flight::Flight(std::shared_ptr<Mesh> mesh,
    const MaterialPbrTextures& textures)
    : flightMesh(mesh)
{
    materialPBR = std::make_shared<Material>();
    materialPBR->SetAllTextures(textures);
}

bool Flight::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
        return false;

    ID3D12Device* device = renderer->GetDevice();

    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC   bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(256);

    if (FAILED(device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&materialConstantBuffer))))
        return false;

    materialConstantBuffer->Map(
        0, nullptr,
        reinterpret_cast<void**>(&mappedMaterialBuffer));

    materialPBR->WriteToGpu(mappedMaterialBuffer);   // �� �� �ʱ� ���ε�
    return true;
}


void Flight::Update(float deltaTime)
{
    const float moveSpeed = 3.0f;

    auto& input = InputManager::GetInstance();
    if (input.IsMouseButtonDown(MouseButton::Left))
    {
        const float sens = 0.002f;
        yaw += input.GetMouseDeltaX() * sens;
        pitch += -input.GetMouseDeltaY() * sens;
        pitch = std::clamp(pitch, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);
        rotation = XMQuaternionRotationRollPitchYaw(pitch, yaw, 0.0f);
    }

    if (input.IsKeyHeld('W')) position.z -= moveSpeed * deltaTime;
    if (input.IsKeyHeld('S')) position.z += moveSpeed * deltaTime;
    if (input.IsKeyHeld('A')) position.x += moveSpeed * deltaTime;
    if (input.IsKeyHeld('D')) position.x -= moveSpeed * deltaTime;
    if (input.IsKeyHeld('E')) position.y += moveSpeed * deltaTime;
    if (input.IsKeyHeld('Q')) position.y -= moveSpeed * deltaTime;

    GameObject::Update(deltaTime);
}


void Flight::Render(Renderer* renderer)
{
    // 1) Direct command list ��������
    ID3D12GraphicsCommandList* directCommandList =
        renderer->GetDirectCommandList();

    // 2) Descriptor Heaps ���ε� (CBV_SRV_UAV + SAMPLER)
    ID3D12DescriptorHeap* descriptorHeaps[] = {
        renderer->GetDescriptorHeapManager()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        renderer->GetDescriptorHeapManager()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    directCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // 3) PBR ���� RS & PSO ���ε�
    auto* rootSignature = renderer->GetRootSignatureManager()->Get(L"PbrRS");
    auto* pipelineState = renderer->GetPSOManager()->Get(L"PbrPSO");
    directCommandList->SetGraphicsRootSignature(rootSignature);
    directCommandList->SetPipelineState(pipelineState);

    // 4) ������ ���� CBV ���ε� (b1: Lighting, b3: Global)
    directCommandList->SetGraphicsRootConstantBufferView(
        1, renderer->GetLightingConstantBuffer()->GetGPUVirtualAddress());
    directCommandList->SetGraphicsRootConstantBufferView(
        3, renderer->GetGlobalConstantBuffer()->GetGPUVirtualAddress());

    // 5) SRV �� Sampler ���̺� ���ε� (root slot 4, 5)
    {
        auto srvStart = descriptorHeaps[0]->GetGPUDescriptorHandleForHeapStart();
        directCommandList->SetGraphicsRootDescriptorTable(4, srvStart);
        auto samplerStart = descriptorHeaps[1]->GetGPUDescriptorHandleForHeapStart();
        directCommandList->SetGraphicsRootDescriptorTable(5, samplerStart);
    }

    // 6) MVP CBV (b0) ������Ʈ & ���ε�
    {
        CB_MVP mvpData{};
        XMStoreFloat4x4(&mvpData.model, XMMatrixTranspose(worldMatrix));
        XMStoreFloat4x4(&mvpData.view, XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix()));
        XMStoreFloat4x4(&mvpData.projection, XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
        XMStoreFloat4x4(&mvpData.modelInvTranspose, XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));
        memcpy(mappedMVPData, &mvpData, sizeof(mvpData));

        directCommandList->SetGraphicsRootConstantBufferView(
            0, constantMVPBuffer->GetGPUVirtualAddress());
    }

    // 7) Material CBV (b2) ������Ʈ & ���ε�
    {
        // mappedMaterialData: CPU���� �����ص� Material CBV �� ������
        materialPBR->WriteToGpu(mappedMaterialBuffer);
        directCommandList->SetGraphicsRootConstantBufferView(
            2, materialConstantBuffer->GetGPUVirtualAddress());
    }

    // 8) IA ���� �� Draw ȣ��
    if (auto mesh = flightMesh.lock())
    {
        directCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        directCommandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        directCommandList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        directCommandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
    }
}