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


    // 2) 초기 worldMatrix를 버퍼에 복사
    constantMVPBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedMVPData));
    memcpy(mappedMVPData, &worldMatrix, sizeof(worldMatrix));
    // Map 상태 유지 후 리소스 해제 시 UnMap
    return true;
}

void GameObject::Update(float deltaTime) {
    // 기본은 WorldMatrix만 갱신 → 서브클래스에서 위치/회전을 바꾸면 
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
