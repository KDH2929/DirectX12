#pragma once

#include "GameObject.h"
#include "Texture.h"
#include "Mesh.h"
#include <DirectXMath.h>
#include <memory>

using namespace DirectX;

class BillboardObject : public GameObject
{
public:
    BillboardObject();
    virtual ~BillboardObject() = default;

    bool Initialize(Renderer* renderer) override;
    void SetTexture(std::shared_ptr<Texture> tex);
    void SetSize(float size);
    void EnableBillboarding(bool enable);
    bool IsBillboardingEnabled() const;
    void SetLifetime(float seconds);
    void ResetElapsedTime();
    bool IsAlive() const;

    virtual void Update(float deltaTime) override;
    virtual void Render(Renderer* renderer) override;

    static std::shared_ptr<Mesh> CreateQuadMesh(ID3D11Device* device);

protected:
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Mesh> quadMesh;

    float size = 1.0f;
    bool enableBillboarding = true;

    float lifetime = -1.0f;
    float elapsedTime = 0.0f;
};