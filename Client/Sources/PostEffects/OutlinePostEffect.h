#pragma once
#include "PostEffect.h"
#include "ConstantBuffers.h"
#include "Mesh.h"

#include <wrl/client.h>
#include <memory>
#include <directx/d3dx12.h> 

class Renderer;
class Mesh;

class OutlinePostEffect : public PostEffect
{
public:
    virtual ~OutlinePostEffect() = default;

    void Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

private:
    // ImGui���� ������ ����
    float thresholdValue = 5.0f;
    float thicknessValue = 1.0f;
    DirectX::XMFLOAT3 outlineColor{ 0.0f, 0.0f, 0.0f };
    float mixFactor = 1.0f;

    // GPU ��� ����
    ComPtr<ID3D12Resource> outlineConstantBuffer;
    CB_OutlineOptions* mappedOutlineOptions = nullptr;

    // Ǯ��ũ�� ���� �޽�
    std::shared_ptr<Mesh> quadMesh;
};