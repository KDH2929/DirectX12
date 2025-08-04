#pragma once

#include "GameObject.h"
#include "Material.h"
#include "Camera.h"
#include <memory>
#include <wrl/client.h>
#include <d3d12.h>

class Renderer;
class Mesh;

class SphereObject : public GameObject
{
public:
    explicit SphereObject(
        std::shared_ptr<Material> material,
        uint32_t latitudeSegments = 16,
        uint32_t longitudeSegments = 16
    );
    ~SphereObject() override = default;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime, Renderer* renderer, UINT objectIndex) override;
    void Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex) override;

private:
    uint32_t latitudeSegments;
    uint32_t longitudeSegments;

    std::shared_ptr<Mesh>     sphereMesh;
    std::shared_ptr<Material> materialPBR;

    float yawCam = 0.0f, pitchCam = 0.0f;
    float yawObj = 0.0f, pitchObj = 0.0f;
    float moveSpeed = 3.0f;

    std::shared_ptr<Camera> camera;
    bool showNormalDebug = false;
};