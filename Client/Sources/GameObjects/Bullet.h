#pragma once

#include "GameObject.h"
#include <memory>

class Bullet : public GameObject
{
public:
    Bullet(const XMFLOAT3& position, const XMFLOAT3& direction, float speed, std::shared_ptr<Mesh> mesh, std::shared_ptr<Texture> texture);

    virtual bool Initialize(Renderer* renderer) override;
    virtual void Update(float deltaTime) override;
    virtual void Render(Renderer* renderer) override;

    bool IsAlive() const { return isAlive; }
    void SetAlive(bool alive) { isAlive = alive; }

    void SetMaxLifeTime(float seconds);

    virtual void OnHit(GameObject* hitTarget);  // 충돌 시 호출

    void Activate(const XMFLOAT3& position, const XMFLOAT3& direction, float speed);
    void Deactivate();

private:
    XMFLOAT3 direction;
    float speed;
    bool isAlive = true;
    float lifeTime = 0.0f;
    float maxLifeTime = 3.0f;

    std::weak_ptr<Mesh> bulletMesh;
};