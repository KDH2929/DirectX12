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
    // ImGui에서 조절할 값들
    float thresholdValue = 5.0f;
    float thicknessValue = 1.0f;
    DirectX::XMFLOAT3 outlineColor{ 0.0f, 0.0f, 0.0f };
    float mixFactor = 1.0f;

    // GPU 상수 버퍼
    ComPtr<ID3D12Resource> outlineConstantBuffer;
    CB_OutlineOptions* mappedOutlineOptions = nullptr;

    // 풀스크린 쿼드 메쉬
    std::shared_ptr<Mesh> quadMesh;
};