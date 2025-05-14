#pragma once
#include "Flight.h"
#include "BillboardExplosion.h"

class EnemyFlight : public Flight {
public:
    EnemyFlight(std::shared_ptr<Mesh> mesh, std::shared_ptr<Texture> texture);
    ~EnemyFlight() override = default;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;

    void SetExplosionEffect(std::shared_ptr<BillboardExplosion> explosion);

    virtual void OnDestroyed() override;

private:
    std::shared_ptr<BillboardExplosion> explosionEffect;
};
