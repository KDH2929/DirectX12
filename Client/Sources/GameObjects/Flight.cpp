#include "Flight.h"
#include "Renderer.h"
#include "Mesh.h"
#include "Texture.h"
#include "DescriptorHeapManager.h"
#include "PipelineStateManager.h"
#include "RootSignatureManager.h"
#include <directx/d3dx12.h>
#include <algorithm>

using namespace DirectX;

Flight::Flight(std::shared_ptr<Mesh> mesh,
    std::shared_ptr<Texture> diffuse,
    std::shared_ptr<Texture> normal)
    : flightMesh(mesh)
    , albedoTexture(diffuse)
    , normalTexture(normal)
{
}

Flight::~Flight()
{
    if (materialConstantBuffer && mappedMaterialPtr)
        materialConstantBuffer->Unmap(0, nullptr);
}

bool Flight::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
        return false;

    ID3D12Device* device = renderer->GetDevice();
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(256);

    if (FAILED(device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&materialConstantBuffer))))
        return false;

    materialConstantBuffer->Map(0, nullptr,
        reinterpret_cast<void**>(&mappedMaterialPtr));

    CB_Material mat{};
    mat.ambient = { 0.1f, 0.1f, 0.1f };
    mat.diffuse = { 0.6f, 0.6f, 0.6f };
    mat.specular = { 0.4f, 0.4f, 0.4f };
    mat.useAlbedoMap = albedoTexture ? 1u : 0u;
    mat.useNormalMap = normalTexture ? 1u : 0u;

    memcpy(mappedMaterialPtr, &mat, sizeof(CB_Material));
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
    auto* cmd = renderer->GetCommandList();
    cmd->SetPipelineState(renderer->GetPSOManager()->Get(L"PhongPSO"));
    cmd->SetGraphicsRootSignature(renderer->GetRootSignatureManager()->Get(L"PhongRS"));

    CB_MVP mvp{};
    XMStoreFloat4x4(&mvp.model, XMMatrixTranspose(worldMatrix));
    XMStoreFloat4x4(&mvp.view, XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix()));
    XMStoreFloat4x4(&mvp.projection, XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
    XMStoreFloat4x4(&mvp.modelInvTranspose, XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));
    memcpy(mappedMVPData, &mvp, sizeof(CB_MVP));

    cmd->SetGraphicsRootConstantBufferView(0, constantMVPBuffer->GetGPUVirtualAddress());
    cmd->SetGraphicsRootConstantBufferView(1, renderer->GetLightingConstantBuffer()->GetGPUVirtualAddress());
    cmd->SetGraphicsRootConstantBufferView(2, materialConstantBuffer->GetGPUVirtualAddress());
    cmd->SetGraphicsRootConstantBufferView(3, renderer->GetGlobalConstantBuffer()->GetGPUVirtualAddress());

    if (albedoTexture)
        cmd->SetGraphicsRootDescriptorTable(4, albedoTexture->GetGpuHandle());
    // normalTexture 는 SRV 힙에서 t0 바로 뒤 슬롯(t1)에 배치되어 있다고 가정

    if (auto mesh = flightMesh.lock())
    {
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        cmd->IASetIndexBuffer(&mesh->GetIndexBufferView());
        cmd->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
    }
}
