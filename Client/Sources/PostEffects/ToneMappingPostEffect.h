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
    void Update(float deltaTime, Renderer* renderer) override;
    void Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer) override;

private:
    // ImGui에서 조절할 값들
    float exposureValue = 4.0f;
    float gammaValue = 1.0f;

    // 풀스크린 쿼드 메쉬
    std::shared_ptr<Mesh> quadMesh;
};
