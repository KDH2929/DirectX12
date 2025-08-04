#include "GameObject.h"
#include "Renderer.h"
#include "FrameResource/FrameResource.h"

GameObject::GameObject() {
}

GameObject::~GameObject()
{
}

bool GameObject::Initialize(Renderer* renderer) {
    return true;
}

void GameObject::Update(float deltaTime, Renderer* renderer, UINT objectIndex) {

    UpdateWorldMatrix();

    CB_MVP constantBufferData{};
    XMStoreFloat4x4(&constantBufferData.model, XMMatrixTranspose(worldMatrix));
    XMStoreFloat4x4(&constantBufferData.view, XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix()));
    XMStoreFloat4x4(&constantBufferData.projection, XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
    XMStoreFloat4x4(&constantBufferData.modelInvTranspose, XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));

    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    assert(frameResource != nullptr && frameResource->cbMVP && "FrameResource or cbMVP is null");

    frameResource->cbMVP->CopyData(objectIndex, constantBufferData);

}

void GameObject::UpdateShadowMap(Renderer* renderer, UINT objectIndex, UINT shadowMapIndex, const XMMATRIX& lightViewProj)
{
    CB_ShadowMapPass data{};
    XMStoreFloat4x4(&data.modelWorld, XMMatrixTranspose(worldMatrix));
    XMStoreFloat4x4(&data.lightViewProj, XMMatrixTranspose(lightViewProj));

    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    assert(frameResource && frameResource->cbShadowPass && "FrameResource or cbShadowPass is null");

    constexpr UINT maxShadowCount = MAX_SHADOW_DSV_COUNT;
    UINT slot = objectIndex * maxShadowCount + shadowMapIndex;
    frameResource->cbShadowPass->CopyData(slot, data);
}

void GameObject::RenderShadowMap(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex, UINT shadowMapIndex) 
{
    auto& frameResource = *renderer->GetCurrentFrameResource();
    assert(frameResource.cbShadowPass && "cbShadowPass is null");

    constexpr UINT maxShadowCount = MAX_SHADOW_DSV_COUNT;
    UINT slot = objectIndex * maxShadowCount + shadowMapIndex;

    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = frameResource.cbShadowPass->GetGPUVirtualAddress(slot);

    commandList->SetGraphicsRootSignature(renderer->GetRootSignatureManager()->Get(L"ShadowMapPassRS"));
    commandList->SetPipelineState(renderer->GetPSOManager()->Get(L"ShadowMapPassPSO"));
    commandList->SetGraphicsRootConstantBufferView(0, gpuAddress);

    if (auto mesh = GetMesh()) {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        commandList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        commandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
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
