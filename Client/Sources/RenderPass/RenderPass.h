#pragma once
#include <d3d12.h>

class Renderer;

class RenderPass
{
public:
    virtual ~RenderPass() = default;
    virtual void Initialize(Renderer* renderer) = 0;
    virtual void Update(float deltaTime, Renderer* renderer) = 0;

    virtual void RenderSingleThreaded(Renderer* renderer) = 0;

};