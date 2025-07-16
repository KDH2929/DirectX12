#pragma once

#include "GameObject.h"
#include "Material.h"
#include "Camera.h"
#include <memory>
#include <wrl/client.h>
#include <d3d12.h>

class Renderer;
class Mesh;

class SphereObject final : public GameObject
{
public:

    explicit SphereObject(
        std::shared_ptr<Material> material,
        uint32_t latitudeSegments = 16,
        uint32_t longitudeSegments = 16);
    ~SphereObject() override;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

private:
    uint32_t latitudeSegments;
    uint32_t longitudeSegments;

    std::shared_ptr<Mesh> sphereMesh;
    std::shared_ptr<Material> materialPBR;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialConstantBuffer;
    std::byte* mappedMaterialBuffer = nullptr;

    float yawCam = 0, pitchCam = 0;
    float yawObj = 0, pitchObj = 0;

    float moveSpeed = 3.0f;

    std::shared_ptr<Camera> camera;

    bool showNormalDebug = false;
};