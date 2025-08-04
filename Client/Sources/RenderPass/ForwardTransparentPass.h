#pragma once

#include "RenderPass.h"
#include <wrl.h>

class Renderer;

class ForwardTransparentPass : public RenderPass
{
public:
    ForwardTransparentPass() = default;
    ~ForwardTransparentPass() override = default;

    void Initialize(Renderer* renderer) override;
    void Update(float deltaTime, Renderer* renderer) override;
    void RenderSingleThreaded(Renderer* renderer) override;
};
