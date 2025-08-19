#pragma once
#include <d3d12.h>
#include <cstdint> 

class Renderer;

class RenderPass
{
public:
    virtual ~RenderPass() = default;
    virtual void Initialize(Renderer* renderer) = 0;
    virtual void Update(float deltaTime, Renderer* renderer) = 0;

    virtual void RenderSingleThreaded(Renderer* renderer) = 0;


    // 멀티스레드용 API
    virtual void RecordPreCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer) {};
    virtual void RecordParallelCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT threadIndex) {};
    virtual void RecordPostCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer) {};


public:

    enum PassIndex : uint8_t {
        ShadowMap = 0,
        ForwardOpaque,
        ForwardTransparent,
        PostProcess,
        Count
    };
};