#pragma once

#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "ConstantBuffers.h"   // CB_MVP, CB_MaterialPBR
#include <wrl/client.h>
#include <memory>
#include <d3d12.h>

class Renderer;

class BoxObject : public GameObject
{
public:
    explicit BoxObject(std::shared_ptr<Material> materialPBR = nullptr);
    ~BoxObject() override = default;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime, Renderer* renderer, UINT objectIndex) override;
    void Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex) override;

private:
    std::shared_ptr<Mesh>     cubeMesh;
    std::shared_ptr<Material> materialPBR;
};