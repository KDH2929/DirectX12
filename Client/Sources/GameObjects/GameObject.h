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

// �ּ����� GameObject: ��ȯ �� ��� ���۸� ����
class GameObject {
public:
    GameObject();
    virtual ~GameObject();

    // �ʱ�ȭ: ��� ���� ����
    virtual bool Initialize(Renderer* renderer);
    // ��ġ/ȸ��/������ ������Ʈ
    virtual void Update(float deltaTime);
    // ������: ��� ���۸� ���ε��ϰ� ��ο� ȣ��
    virtual void Render(Renderer* renderer) = 0;

    // ��ȯ ����
    void SetPosition(const XMFLOAT3& pos);
    void SetScale(const XMFLOAT3& scale);
    void SetRotationQuat(const XMVECTOR& quat);

protected:
    // ���� ��� ���
    void UpdateWorldMatrix();

    // D3D12 ��� ����
    ComPtr<ID3D12Resource>       constantMVPBuffer;
    UINT8* mappedMVPData = nullptr;

    // ��ȯ ����
    XMFLOAT3 position    = {0,0,0};
    XMFLOAT3 scale       = {1,1,1};
    XMVECTOR rotation    = XMQuaternionIdentity();
    XMMATRIX worldMatrix = XMMatrixIdentity();
};
