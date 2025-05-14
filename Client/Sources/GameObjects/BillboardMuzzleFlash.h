#pragma once

#include "BillboardObject.h"
#include <d3d11.h>
#include <wrl/client.h>

class BillboardMuzzleFlash : public BillboardObject
{
public:
    BillboardMuzzleFlash();
    virtual ~BillboardMuzzleFlash();

    bool Initialize(Renderer* renderer, std::shared_ptr<Texture> texture, float size, float lifetime);
    void SetFadeOut(bool enable);

    virtual void Update(float deltaTime) override;
    virtual void Render(Renderer* renderer) override;

private:
    bool CreateBlendState(ID3D11Device* device);
    bool CreateRasterizerState(ID3D11Device* device);

private:
    bool fadeOut = true;
};
