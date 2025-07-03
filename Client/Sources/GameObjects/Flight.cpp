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
    std::shared_ptr<Texture> texture)
    : flightMesh(mesh), diffuseTexture(texture)
{
    /*
    const auto& vbv = mesh->GetVertexBufferView();
    const auto& ibv = mesh->GetIndexBufferView();
    uint32_t idxCount = mesh->GetIndexCount();

    assert(vbv.BufferLocation != 0 && "VB BufferLocation is ZERO!");
    assert(vbv.SizeInBytes != 0 && "VB SizeInBytes is ZERO!");
    assert(vbv.StrideInBytes != 0 && "VB StrideInBytes is ZERO!");
    assert(ibv.BufferLocation != 0 && "IB BufferLocation is ZERO!");
    assert(ibv.SizeInBytes != 0 && "IB SizeInBytes is ZERO!");
    assert(idxCount > 0 && "indexCount is ZERO!");
    */
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

    // Material CBV (256 B Upload)
    ID3D12Device* device = renderer->GetDevice();
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC   bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(256);

    if (FAILED(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&materialConstantBuffer))))
        return false;

    materialConstantBuffer->Map(0, nullptr,
        reinterpret_cast<void**>(&mappedMaterialPtr));

    // 기본 머티리얼(연회색)
    CB_Material material{};
    material.ambient = { 0.1f, 0.1f, 0.1f };
    material.diffuse = { 0.6f, 0.6f, 0.6f };
    material.specular = { 0.4f, 0.4f, 0.4f };
    material.useTexture = diffuseTexture ? 1 : 0;
    memcpy(mappedMaterialPtr, &material, sizeof(CB_Material));

    return true;
}


void Flight::Update(float deltaTime)
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


    GameObject::Update(deltaTime);
}

void Flight::Render(Renderer* renderer)
{
    auto commandList = renderer->GetCommandList();

    // PSO & RootSignature
    commandList->SetPipelineState(
        renderer->GetPSOManager()->Get(L"PhongPSO"));
    commandList->SetGraphicsRootSignature(
        renderer->GetRootSignatureManager()->Get(L"PhongRS"));

    // MVP 상수 버퍼 업데이트
    CB_MVP mvp{};
    XMStoreFloat4x4(&mvp.model, XMMatrixTranspose(worldMatrix));
    XMStoreFloat4x4(&mvp.view,
        XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix()));
    XMStoreFloat4x4(&mvp.projection,
        XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
    XMStoreFloat4x4(&mvp.modelInvTranspose,
        XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));
    memcpy(mappedMVPData, &mvp, sizeof(CB_MVP));

    // Root CBV (b0~b3)
    commandList->SetGraphicsRootConstantBufferView(
        0, constantMVPBuffer->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(
        1, renderer->GetLightingConstantBuffer()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(
        2, materialConstantBuffer->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(
        3, renderer->GetGlobalConstantBuffer()->GetGPUVirtualAddress());

    // 텍스처 SRV (t0)
    if (diffuseTexture)
    {
        commandList->SetGraphicsRootDescriptorTable(
            4, diffuseTexture->GetGpuHandle());
    }

    // Sampler(s0)는 렌더러가 프레임마다 slot 5에 미리 바인딩한다 가정.

    // Mesh 드로우
    if (auto mesh = flightMesh.lock())
    {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        commandList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        commandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
    }
}
