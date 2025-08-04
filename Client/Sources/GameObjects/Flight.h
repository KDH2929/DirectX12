#pragma once
#include "GameObject.h"
#include "Material.h"
#include "Camera.h"
#include <memory>

class Mesh;
class Renderer;

class Flight : public GameObject
{
public:
    Flight(std::shared_ptr<Mesh> mesh,
        const MaterialPbrTextures& textures);

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime, Renderer* renderer, UINT objectIndex) override;
    void Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex) override;

private:
    std::weak_ptr<Mesh> flightMesh;
    std::shared_ptr<Material> materialPBR;

    std::shared_ptr<Camera> camera;
    float yawCam = 0, pitchCam = 0;
    bool showNormalDebug = false;
};