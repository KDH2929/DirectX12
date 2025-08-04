#pragma once

#include "RenderPass.h"
#include <wrl.h>
#include <vector>
#include <memory>

class Renderer;
class PostEffect;

class PostProcessPass : public RenderPass
{
public:
    PostProcessPass() = default;
    ~PostProcessPass() override = default;

    void Initialize(Renderer* renderer) override;
    void Update(float deltaTime, Renderer* renderer) override;
    void RenderSingleThreaded(Renderer* renderer) override;


private:
    std::vector<std::shared_ptr<PostEffect>> postEffects;
};