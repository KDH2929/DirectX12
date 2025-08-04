#pragma once

#include <d3d12.h>
#include <directx/d3dx12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "InputManager.h"
#include "Mesh.h"
#include "ConstantBuffers.h"
#include "ShadowMap.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class Renderer;

class GameObject {
public:
    GameObject();
    virtual ~GameObject();

    virtual bool Initialize(Renderer* renderer);
    virtual void Update(float deltaTime, Renderer* renderer = nullptr, UINT objectIndex = 0);
    virtual void Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex) = 0;

    void UpdateShadowMap(Renderer* renderer, UINT objectIndex, UINT shadowMapIndex, const XMMATRIX& lightViewProj);
    void RenderShadowMap(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex, UINT shadowMapIndex);

    void SetPosition(const XMFLOAT3& pos);
    void SetScale(const XMFLOAT3& scale);
    void SetRotationQuat(const XMVECTOR& quat);

    void  SetTransparent(bool isTransparent);
    bool  IsTransparent() const;

    float DistanceToCamera(const XMVECTOR& cameraPos) const;

    void SetMesh(std::shared_ptr<Mesh> mesh);
    std::shared_ptr<Mesh> GetMesh() const;

protected:
    void UpdateWorldMatrix();

    // ��ȯ ����
    XMFLOAT3 position    = {0,0,0};
    XMFLOAT3 scale       = {1,1,1};
    XMVECTOR rotation    = XMQuaternionIdentity();
    XMMATRIX worldMatrix = XMMatrixIdentity();

    bool transparent = false;

    std::shared_ptr<Mesh> mesh;     // ��� GameObject�� 1���� �޽��� ���´ٰ� ����
};
