#pragma once

#include "BillboardObject.h"
#include <d3d11.h>
#include <wrl/client.h>

class BillboardMuzzleSmoke : public BillboardObject
{
public:
    BillboardMuzzleSmoke();
    virtual ~BillboardMuzzleSmoke();

    bool Initialize(Renderer* renderer, std::shared_ptr<Texture> mainTexture, std::shared_ptr<Texture> noiseTexture, float size, float lifetime);
    void SetFadeOut(bool enable);

    virtual void Update(float deltaTime) override;
    virtual void Render(Renderer* renderer) override;

private:
    bool CreateBlendState(ID3D11Device* device);
    bool CreateRasterizerState(ID3D11Device* device);
    bool CreateDepthState(ID3D11Device* device);

private:
    bool fadeOut = true;
    std::shared_ptr<Texture> noiseTexture;
};
