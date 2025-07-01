#include "BoxObject.h"
#include "Renderer.h"
#include "DescriptorHeapManager.h"
#include "PipelineStateManager.h"
#include "RootSignatureManager.h"
#include <directx/d3dx12.h>
#include <algorithm>

using namespace DirectX;

BoxObject::BoxObject()
{
    position = { 0.f, 0.f, 0.f };
    scale = { 1.f, 1.f, 1.f };
    rotation = XMQuaternionIdentity();
}

BoxObject::~BoxObject()
{
    if (materialConstantBuffer && mappedMaterialPtr)
        materialConstantBuffer->Unmap(0, nullptr);
}

// ---------------------------------------------------------------------------
bool BoxObject::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
        return false;

    // 1) Cube geometry
    cubeMesh = Mesh::CreateCube(renderer);
    if (!cubeMesh) return false;

    // 2) Create material constant buffer (256B aligned Upload)
    ID3D12Device* device = renderer->GetDevice();
    CD3DX12_HEAP_PROPERTIES uploadProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC   bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(256);
    if (FAILED(device->CreateCommittedResource(&uploadProps, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&materialConstantBuffer))))
        return false;

    materialConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedMaterialPtr));

    // 3) Write initial CB_Material data (red Phong)
    CB_Material material{};
    material.ambient = { 0.2f, 0.0f, 0.0f };
    material.diffuse = { 1.0f, 0.0f, 0.0f };
    material.specular = { 0.4f, 0.4f, 0.4f };
    material.useTexture = 0;
    memcpy(mappedMaterialPtr, &material, sizeof(CB_Material));

    return true;
}


void BoxObject::Update(float deltaTime)
{
    const float moveSpeed = 3.0f;          // 1초에 3유닛

    auto& input = InputManager::GetInstance();

    if (input.IsMouseButtonDown(MouseButton::Left))
    {
        const float sens = 0.002f;
        yaw += input.GetMouseDeltaX() * sens;
        pitch += -input.GetMouseDeltaY() * sens;
        pitch = std::clamp(pitch, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);

        rotation = XMQuaternionRotationRollPitchYaw(pitch, yaw, 0.0f);
    }


    // ―― 평면 이동 (XZ)
    if (input.IsKeyHeld('W')) position.z -= moveSpeed * deltaTime; // 앞
    if (input.IsKeyHeld('S')) position.z += moveSpeed * deltaTime; // 뒤
    if (input.IsKeyHeld('A')) position.x += moveSpeed * deltaTime; // 좌
    if (input.IsKeyHeld('D')) position.x -= moveSpeed * deltaTime; // 우

    // ―― 수직 이동 (Y)
    if (input.IsKeyHeld('E')) position.y += moveSpeed * deltaTime;   // 위
    if (input.IsKeyHeld('Q')) position.y -= moveSpeed * deltaTime;   // 아래


    // 부모 클래스에서 WorldMatrix 갱신
    GameObject::Update(deltaTime);
}


void BoxObject::Render(Renderer* renderer)
{
    ID3D12GraphicsCommandList* cmd = renderer->GetCommandList();

    // PSO & RootSignature
    cmd->SetPipelineState(renderer->GetPSOManager()->Get(L"PhongPSO"));
    cmd->SetGraphicsRootSignature(renderer->GetRootSignatureManager()->Get(L"PhongRS"));

    // Update MVP
    CB_MVP mvp{};
    XMStoreFloat4x4(&mvp.model, XMMatrixTranspose(worldMatrix));
    XMStoreFloat4x4(&mvp.view, XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix()));
    XMStoreFloat4x4(&mvp.projection, XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
    XMStoreFloat4x4(&mvp.modelInvTranspose,
        XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));
    memcpy(mappedMVPData, &mvp, sizeof(CB_MVP));

    // Root CBV bindings (b0 ~ b3)
    cmd->SetGraphicsRootConstantBufferView(0, constantMVPBuffer->GetGPUVirtualAddress());
    cmd->SetGraphicsRootConstantBufferView(1, renderer->GetLightingConstantBuffer()->GetGPUVirtualAddress());
    cmd->SetGraphicsRootConstantBufferView(2, materialConstantBuffer->GetGPUVirtualAddress());
    cmd->SetGraphicsRootConstantBufferView(3, renderer->GetGlobalConstantBuffer()->GetGPUVirtualAddress());

    // Draw
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->IASetVertexBuffers(0, 1, &cubeMesh->GetVertexBufferView());
    cmd->IASetIndexBuffer(&cubeMesh->GetIndexBufferView());
    cmd->DrawIndexedInstanced(cubeMesh->GetIndexCount(), 1, 0, 0, 0);
}
