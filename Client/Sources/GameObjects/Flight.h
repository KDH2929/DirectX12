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
        std::shared_ptr<Texture> texture);
    ~Flight() override;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime)       override;
    void Render(Renderer* renderer)    override;

private:
    // ������ �ڿ�
    std::weak_ptr<Mesh>       flightMesh;
    std::shared_ptr<Texture>  diffuseTexture;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialConstantBuffer;
    UINT8* mappedMaterialPtr = nullptr;

    float yaw = 0.0f;   // ����
    float pitch = 0.0f; // ����
};
