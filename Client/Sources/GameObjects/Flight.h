#pragma once

#include "GameObject.h"
#include <memory>

class Mesh;
class Texture;
class Renderer;

class Flight : public GameObject
{
public:
    Flight(std::shared_ptr<Mesh> mesh,
        std::shared_ptr<Texture> diffuse,
        std::shared_ptr<Texture> normal = nullptr);
    ~Flight() override;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

private:
    // resources
    std::weak_ptr<Mesh>      flightMesh;
    std::shared_ptr<Texture> albedoTexture;   // t0
    std::shared_ptr<Texture> normalTexture;   // t1

    Microsoft::WRL::ComPtr<ID3D12Resource> materialConstantBuffer;
    UINT8* mappedMaterialPtr = nullptr;

    float yaw = 0.0f;
    float pitch = 0.0f;
};
