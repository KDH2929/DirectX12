#pragma once

#include "GameObject.h"

class Particle : public GameObject
{
public:
    Particle();
    virtual ~Particle() = default;

    void SetLifetime(float seconds);
    bool IsAlive() const;

    void SetFadeOut(bool enable);
    void SetSize(float size);
    float GetAlpha() const;

    virtual void Update(float deltaTime) override;
    virtual void Render(Renderer* renderer) override = 0;

protected:
    float size = 1.0f;
    float lifetime = -1.0f;
    float elapsedTime = 0.0f;
    bool fadeOut = true;

    float alpha = 1.0f;
};
