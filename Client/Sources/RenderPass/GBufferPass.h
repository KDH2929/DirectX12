#pragma once

#include "RenderPass.h"
#include <wrl.h>

class Renderer;

// G-Buffer 패스: 위치, 법선, 재질 정보 등을 렌더 타깃에 기록
class GBufferPass : public RenderPass
{
public:
    GBufferPass() = default;
    virtual ~GBufferPass() = default;

    void Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;
};