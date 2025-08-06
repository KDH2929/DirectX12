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

    void RecordPreCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer) override;
    void RecordParallelCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT threadIndex) override;
    void RecordPostCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer) override;

private:
    std::vector<std::shared_ptr<PostEffect>> postEffects;
};