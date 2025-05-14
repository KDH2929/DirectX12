#pragma once

#include "GameObject.h"

class Flight : public GameObject {
public:
    Flight(std::shared_ptr<Mesh> mesh, std::shared_ptr<Texture> texture);
    ~Flight() override = default;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

    // 체력관련
    void SetHealth(float hp);
    float GetHealth() const;
    void TakeDamage(float damage);
    virtual void OnDestroyed();

    // 생존 상태 관리
    void SetAlive(bool alive);
    bool IsAlive() const;


    // 적/아군 플래그
    void SetIsEnemy(bool enemy);
    bool IsEnemy() const;


private:

    std::weak_ptr<Mesh> flightMesh;

    // 체력
    float health = 100.0f;
    float maxHealth = 100.0f;

    // 적 여부
    bool isEnemy = false;
    bool isAlive = true;

};
