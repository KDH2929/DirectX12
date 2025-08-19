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


    // 멀티스레드용 API
    void RecordPreCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer) override;
    void RecordParallelCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT threadIndex) override;
    void RecordPostCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer) override;

};