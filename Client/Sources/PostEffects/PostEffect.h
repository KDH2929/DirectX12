#pragma once

class Renderer;

class PostEffect
{
public:
    virtual ~PostEffect() = default;
    virtual void Initialize(Renderer* renderer) = 0;
    virtual void Update(float deltaTime, Renderer* renderer) = 0;
    virtual void Render(Renderer* renderer) = 0;
};