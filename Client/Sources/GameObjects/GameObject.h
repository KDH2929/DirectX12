#pragma once

#include <d3d12.h>
#include <directx/d3dx12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "InputManager.h"

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

    void SetPosition(const XMFLOAT3& pos);
    void SetScale(const XMFLOAT3& scale);
    void SetRotationQuat(const XMVECTOR& quat);

    void  SetTransparent(bool isTransparent);
    bool  IsTransparent() const;

    float DistanceToCamera(const XMVECTOR& cameraPos) const;


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
};
