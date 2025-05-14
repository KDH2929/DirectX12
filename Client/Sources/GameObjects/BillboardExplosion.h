#pragma once

#include "BillboardObject.h"
#include <d3d11.h>
#include <wrl/client.h>

class BillboardExplosion : public BillboardObject
{
public:
    BillboardExplosion();
    virtual ~BillboardExplosion();

    bool Initialize(Renderer* renderer, std::shared_ptr<Texture> spriteSheet, int rows, int cols, float duration);
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

    void Activate();
    void DeActivate();

private:
    int rows = 0;
    int cols = 0;
    int totalFrames = 0;
    float frameDuration = 0.0f;
    float frameTime = 0.0f;
    int currentFrame = 0;

    float uvWidth = 0.0f;
    float uvHeight = 0.0f;

    XMFLOAT2 uvRect[4];

    struct CB_UVRect
    {
        XMFLOAT2 uvRect0;
        XMFLOAT2 uvRect1;
        XMFLOAT2 uvRect2;
        XMFLOAT2 uvRect3;
    };

    ComPtr<ID3D11Buffer> uvRectConstantBuffer;

    bool CreateBlendState(ID3D11Device* device);
    bool CreateRasterizerState(ID3D11Device* device);

    void CreateUVRectBuffer(ID3D11Device* device);
};
