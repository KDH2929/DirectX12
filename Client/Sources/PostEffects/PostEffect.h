#pragma once
#include <d3d12.h>

class Renderer;

class PostEffect
{
public:
    virtual ~PostEffect() = default;
    virtual void Initialize(Renderer* renderer) = 0;
    virtual void Update(float deltaTime, Renderer* renderer) = 0;
    virtual void Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer) = 0;
};