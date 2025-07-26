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

bool BoxObject::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
        return false;

    // 1) Cube geometry
    cubeMesh = Mesh::CreateCube(renderer);
    if (!cubeMesh) return false;

    SetMesh(cubeMesh);

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
    CB_MaterialPhong material{};
    material.ambient = { 0.2f, 0.0f, 0.0f };
    material.diffuse = { 1.0f, 0.0f, 0.0f };
    material.specular = { 0.4f, 0.4f, 0.4f };
    material.useAlbedoMap = 0;
    memcpy(mappedMaterialPtr, &material, sizeof(CB_MaterialPhong));

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


    // 평면 이동 (XZ)
    if (input.IsKeyHeld('W')) position.z -= moveSpeed * deltaTime; // 앞
    if (input.IsKeyHeld('S')) position.z += moveSpeed * deltaTime; // 뒤
    if (input.IsKeyHeld('A')) position.x += moveSpeed * deltaTime; // 좌
    if (input.IsKeyHeld('D')) position.x -= moveSpeed * deltaTime; // 우

    // 수직 이동 (Y)
    if (input.IsKeyHeld('E')) position.y += moveSpeed * deltaTime;   // 위
    if (input.IsKeyHeld('Q')) position.y -= moveSpeed * deltaTime;   // 아래


    // 부모 클래스에서 WorldMatrix 갱신
    GameObject::Update(deltaTime);
}


void BoxObject::Render(Renderer* renderer)
{
    // 1) Direct 커맨드 리스트 가져오기
    ID3D12GraphicsCommandList* directCommandList =
        renderer->GetDirectCommandList();

    // 2) Descriptor Heaps 바인딩 (CBV_SRV_UAV + SAMPLER)
    ID3D12DescriptorHeap* descriptorHeaps[] = {
        renderer->GetDescriptorHeapManager()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        renderer->GetDescriptorHeapManager()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    directCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // 3) 루트 시그니처 및 PSO 바인딩
    auto* rootSignature = renderer->GetRootSignatureManager()->Get(L"PhongRS");
    auto* pipelineState = renderer->GetPSOManager()->Get(L"PhongPSO");
    directCommandList->SetGraphicsRootSignature(rootSignature);
    directCommandList->SetPipelineState(pipelineState);

    // 4) 프레임 전역 CBV 바인딩 (b1: Lighting, b3: Global)
    directCommandList->SetGraphicsRootConstantBufferView(1, renderer->GetLightingManager()->GetConstantBuffer()->GetGPUVirtualAddress());
    directCommandList->SetGraphicsRootConstantBufferView(3, renderer->GetGlobalConstantBuffer()->GetGPUVirtualAddress());

    // 5) SRV 및 Sampler 테이블 바인딩 (root slot 4, 5)
    D3D12_GPU_DESCRIPTOR_HANDLE srvStart = descriptorHeaps[0]->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE samplerStart = descriptorHeaps[1]->GetGPUDescriptorHandleForHeapStart();
    directCommandList->SetGraphicsRootDescriptorTable(4, srvStart);
    directCommandList->SetGraphicsRootDescriptorTable(5, samplerStart);

    // 6) MVP 및 역전치 모델 행렬 계산 후 상수 버퍼에 복사
    CB_MVP mvpData{};
    XMStoreFloat4x4(&mvpData.model, XMMatrixTranspose(worldMatrix));
    XMStoreFloat4x4(&mvpData.view, XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix()));
    XMStoreFloat4x4(&mvpData.projection, XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
    XMStoreFloat4x4(&mvpData.modelInvTranspose,
        XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));
    memcpy(mappedMVPData, &mvpData, sizeof(mvpData));

    // 7) 객체별 CBV 바인딩 (b0: MVP, b2: Material)
    directCommandList->SetGraphicsRootConstantBufferView(
        0, constantMVPBuffer->GetGPUVirtualAddress());
    directCommandList->SetGraphicsRootConstantBufferView(
        2, materialConstantBuffer->GetGPUVirtualAddress());

    // 8) IA 설정 및 Draw 호출
    directCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    directCommandList->IASetVertexBuffers(0, 1, &cubeMesh->GetVertexBufferView());
    directCommandList->IASetIndexBuffer(&cubeMesh->GetIndexBufferView());
    directCommandList->DrawIndexedInstanced(cubeMesh->GetIndexCount(), 1, 0, 0, 0);
}