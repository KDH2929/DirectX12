#pragma once

#include "RenderPass.h"
#include <wrl.h>

class Renderer;

// G-Buffer �н�: ��ġ, ����, ���� ���� ���� ���� Ÿ�꿡 ���
class GBufferPass : public RenderPass
{
public:
    GBufferPass() = default;
    virtual ~GBufferPass() = default;

    void Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;
};