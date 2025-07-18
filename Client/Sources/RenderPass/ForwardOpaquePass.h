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
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;
};
