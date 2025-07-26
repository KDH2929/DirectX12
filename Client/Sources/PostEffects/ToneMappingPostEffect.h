#pragma once

#include "PostEffect.h"
#include "ConstantBuffers.h"
#include "Mesh.h"

#include <wrl/client.h>
#include <memory>
#include <directx/d3dx12.h>

class Renderer;
class Mesh;

class ToneMappingPostEffect : public PostEffect
{
public:
    virtual ~ToneMappingPostEffect() = default;

    void Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

private:
    // ImGui���� ������ ����
    float exposureValue = 4.0f;
    float gammaValue = 1.0f;

    // GPU ��� ����
    ComPtr<ID3D12Resource> toneMapConstantBuffer;
    CB_ToneMapping* mappedToneMapOptions = nullptr;

    // Ǯ��ũ�� ���� �޽�
    std::shared_ptr<Mesh> quadMesh;
};
