#pragma once
#include "GameObject.h"
#include "Material.h"
#include <memory>

class Mesh;
class Renderer;

class Flight : public GameObject
{
public:
    Flight(std::shared_ptr<Mesh> mesh,
        const MaterialPbrTextures& textures);

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

private:
    std::weak_ptr<Mesh> flightMesh;
    std::shared_ptr<Material> materialPBR;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialConstantBuffer;
    std::byte* mappedMaterialBuffer = nullptr;

    float yaw = 0.f;
    float pitch = 0.f;
};