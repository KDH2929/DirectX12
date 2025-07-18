#pragma once

class Renderer;

class RenderPass
{
public:
    virtual ~RenderPass() = default;
    virtual void Initialize(Renderer* renderer) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render(Renderer* renderer) = 0;
};