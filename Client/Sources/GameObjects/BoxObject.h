#pragma once

#include "GameObject.h"
#include "Mesh.h"
#include "ConstantBuffers.h"   // CB_Material, CB_MVP, …
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

class Renderer;

class BoxObject : public GameObject
{
public:
    BoxObject();
    virtual ~BoxObject();

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

private:
    std::shared_ptr<Mesh> cubeMesh;

    // Material constant buffer (Upload heap, mirrors CB_Material layout)
    ComPtr<ID3D12Resource> materialConstantBuffer;
    UINT8* mappedMaterialPtr = nullptr;


    float yaw = 0.0f;   // 라디안
    float pitch = 0.0f; // 라디안
};
