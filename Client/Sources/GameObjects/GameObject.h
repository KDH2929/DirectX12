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

// 최소한의 GameObject: 변환 및 상수 버퍼만 관리
class GameObject {
public:
    GameObject();
    virtual ~GameObject();

    // 초기화: 상수 버퍼 생성
    virtual bool Initialize(Renderer* renderer);
    // 위치/회전/스케일 업데이트
    virtual void Update(float deltaTime);
    // 렌더링: 상수 버퍼를 바인딩하고 드로우 호출
    virtual void Render(Renderer* renderer) = 0;

    // 변환 설정
    void SetPosition(const XMFLOAT3& pos);
    void SetScale(const XMFLOAT3& scale);
    void SetRotationQuat(const XMVECTOR& quat);

protected:
    // 월드 행렬 계산
    void UpdateWorldMatrix();

    // D3D12 상수 버퍼
    ComPtr<ID3D12Resource>       constantMVPBuffer;
    UINT8* mappedMVPData = nullptr;

    // 변환 정보
    XMFLOAT3 position    = {0,0,0};
    XMFLOAT3 scale       = {1,1,1};
    XMVECTOR rotation    = XMQuaternionIdentity();
    XMMATRIX worldMatrix = XMMatrixIdentity();
};
