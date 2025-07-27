#pragma once

#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "ConstantBuffers.h"   // CB_MVP, CB_MaterialPBR
#include <wrl/client.h>
#include <memory>
#include <d3d12.h>

class Renderer;

class BoxObject final : public GameObject
{
public:
    explicit BoxObject(std::shared_ptr<Material> materialPBR = nullptr);
    ~BoxObject() override;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

private:
    std::shared_ptr<Mesh>            cubeMesh;
    std::shared_ptr<Material>        materialPBR;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialConstantBuffer;
    std::byte* mappedMaterialBuffer = nullptr;

    bool                              showNormalDebug = false;
};