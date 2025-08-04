#pragma once

#include "RenderPass.h"
#include <wrl.h>

class Renderer;

class ForwardOpaquePass : public RenderPass
{
public:
    ForwardOpaquePass() = default;
    ~ForwardOpaquePass() override = default;

    void Initialize(Renderer* renderer) override;
    void Update(float deltaTime, Renderer* renderer) override;
    void RenderSingleThreaded(Renderer* renderer) override;

};