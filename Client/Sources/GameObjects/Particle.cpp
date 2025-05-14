#include "Particle.h"

Particle::Particle() {}

void Particle::SetLifetime(float seconds)
{
    lifetime = seconds;
    elapsedTime = 0.0f;
}

bool Particle::IsAlive() const
{
    return (lifetime < 0.0f) || (elapsedTime < lifetime);
}

void Particle::SetFadeOut(bool enable)
{
    fadeOut = enable;
}

void Particle::SetSize(float newSize)
{
    size = newSize;
}

float Particle::GetAlpha() const
{
    return alpha;
}

void Particle::Update(float deltaTime)
{
    elapsedTime += deltaTime;

    if (fadeOut && lifetime > 0.0f)
        alpha = std::max<float>(0.0f, 1.0f - (elapsedTime / lifetime));
    else
        alpha = 1.0f;
}
