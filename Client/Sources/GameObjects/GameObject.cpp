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
    // 1) CBV�� ���ε� �� ���� ����
    auto device = renderer->GetDevice();
    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC   desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMFLOAT4X4));

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constantMVPBuffer));
    if (FAILED(hr)) return false;


    // 2) �ʱ� worldMatrix�� ���ۿ� ����
    constantMVPBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedMVPData));
    memcpy(mappedMVPData, &worldMatrix, sizeof(worldMatrix));
    // Map ���� ���� �� ���ҽ� ���� �� UnMap
    return true;
}

void GameObject::Update(float deltaTime) {
    // �⺻�� WorldMatrix�� ���� �� ����Ŭ�������� ��ġ/ȸ���� �ٲٸ� 
    UpdateWorldMatrix();
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

void GameObject::UpdateWorldMatrix() {
    XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
    XMMATRIX R = XMMatrixRotationQuaternion(rotation);
    XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);
    worldMatrix = S * R * T;
}
