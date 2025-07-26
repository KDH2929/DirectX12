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
    virtual void Update(float deltaTime);
    virtual void Render(Renderer* renderer) = 0;
    void RenderShadowMapPass(Renderer* renderer, const XMMATRIX& lightViewProj, int shadowMapIndex);

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

    // D3D12 상수 버퍼
    ComPtr<ID3D12Resource>       constantMVPBuffer;
    UINT8* mappedMVPData = nullptr;

    // 변환 정보
    XMFLOAT3 position    = {0,0,0};
    XMFLOAT3 scale       = {1,1,1};
    XMVECTOR rotation    = XMQuaternionIdentity();
    XMMATRIX worldMatrix = XMMatrixIdentity();

    bool transparent = false;

    std::shared_ptr<Mesh> mesh;     // 모든 GameObject는 1개의 메쉬를 갖는다고 가정


    // ShadowMapPass ConstantBuffers
    ComPtr<ID3D12Resource> shadowMapConstantBuffers[MAX_SHADOW_DSV_COUNT];
    UINT8* mappedShadowMapPtrs[MAX_SHADOW_DSV_COUNT] = {};
};
