#pragma once

#include "RenderPass.h"
#include <wrl.h>

class Renderer;

class ShadowMapPass : public RenderPass
{
public:
    ShadowMapPass() = default;
    ~ShadowMapPass() override = default;

    void Initialize(Renderer* renderer) override;
    void Update(float deltaTime, Renderer* renderer) override;
    void RenderSingleThreaded(Renderer* renderer) override;

    // 멀티스레드용 API
    void RecordParallelCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT threadIndex) override;

};
