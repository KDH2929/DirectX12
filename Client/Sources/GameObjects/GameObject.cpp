#include "GameObject.h"
#include "Renderer.h"

GameObject::GameObject() {
}

GameObject::~GameObject()
{
    if (constantMVPBuffer) {
        // Unmap the upload heap before releasing the resource
        constantMVPBuffer->Unmap(0, nullptr);
    }

}

bool GameObject::Initialize(Renderer* renderer) {
    // 1) CBV용 업로드 힙 버퍼 생성
    auto device = renderer->GetDevice();

    // CB_MVP 전체 크기(4x4 행렬 4개 = 256바이트)를 256바이트 경계로 맞춤
    // 항상 올림하여 가장 가까운 256의 배수로 맞춰줌
    // sizeof(CB_MVP)가 192라면 
    // 192 + 255 = 447
    // 447 & ~255 = 447 & 0xFFFFFFFFFFFFFF00 = 256
    
    // sizeof(CB_MVP)가 300이라면
    // (300 + 255) & ~255 = 555 & ~255 = 512

    UINT64 rawSize = sizeof(CB_MVP);
    UINT64 alignedSize = (rawSize + 255) & ~static_cast<UINT64>(255);

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize);

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constantMVPBuffer));
    if (FAILED(hr)) {
        return false;
    }

    // 2) 초기 worldMatrix를 버퍼에 복사
    constantMVPBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedMVPData));
    memcpy(mappedMVPData, &worldMatrix, sizeof(worldMatrix));
    // Map 상태 유지 후 리소스 해제 시 UnMap



     // ShadowMap Constant Buffer
    for (int i = 0; i < MAX_SHADOW_DSV_COUNT; ++i)
    {
        UINT64 rawSize = sizeof(CB_MVP);
        UINT64 alignedSize = (rawSize + 255) & ~static_cast<UINT64>(255);

        CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize);

        ThrowIfFailed(device->CreateCommittedResource(
            &props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&shadowMapConstantBuffers[i])));

        shadowMapConstantBuffers[i]->Map(0, nullptr,
            reinterpret_cast<void**>(&mappedShadowMapPtrs[i]));
    }

    return true;
}

void GameObject::Update(float deltaTime) {
    // 기본은 WorldMatrix만 갱신 → 서브클래스에서 위치/회전을 바꾸면 
    UpdateWorldMatrix();
}

void GameObject::RenderShadowMapPass(Renderer* renderer, const XMMATRIX& lightViewProj, int shadowMapIndex)
{
    auto directCommandList = renderer->GetDirectCommandList();

    CB_ShadowMapPass cb{};
    cb.modelWorld = XMMatrixTranspose(worldMatrix);
    cb.lightViewProj = XMMatrixTranspose(lightViewProj);

    memcpy(mappedShadowMapPtrs[shadowMapIndex], &cb, sizeof(cb));

    directCommandList->SetGraphicsRootSignature(renderer->GetRootSignatureManager()->Get(L"ShadowMapPassRS"));
    directCommandList->SetGraphicsRootConstantBufferView(0, shadowMapConstantBuffers[shadowMapIndex]->GetGPUVirtualAddress());
    directCommandList->SetPipelineState(renderer->GetPSOManager()->Get(L"ShadowMapPassPSO"));

    if (auto mesh = GetMesh())
    {
        directCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        directCommandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        directCommandList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        directCommandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
    }
}

void GameObject::SetPosition(const XMFLOAT3& pos) {
    position = pos;
}

void GameObject::SetScale(const XMFLOAT3& s) {
    scale = s;
}

void GameObject::SetRotationQuat(const XMVECTOR& quat) {
    rotation = XMQuaternionNormalize(quat);
}

void GameObject::SetTransparent(bool isTransparent)
{
    transparent = isTransparent;
}

bool GameObject::IsTransparent() const
{
    return transparent;
}

float GameObject::DistanceToCamera(const XMVECTOR& cameraPos) const
{
    XMVECTOR worldPos = XMLoadFloat3(&position);
    return XMVectorGetX(XMVector3Length(worldPos - cameraPos));
}

void GameObject::SetMesh(std::shared_ptr<Mesh> mesh_)
{
    mesh = mesh_;
}

std::shared_ptr<Mesh> GameObject::GetMesh() const
{
    return mesh;
}

void GameObject::UpdateWorldMatrix() {
    XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
    XMMATRIX R = XMMatrixRotationQuaternion(rotation);
    XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);
    worldMatrix = S * R * T;
}
