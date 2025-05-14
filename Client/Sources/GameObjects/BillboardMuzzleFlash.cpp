#include "BillboardMuzzleFlash.h"
#include "Renderer.h"

BillboardMuzzleFlash::BillboardMuzzleFlash() {}

BillboardMuzzleFlash::~BillboardMuzzleFlash()
{
    blendState.Reset();
    rasterizerState.Reset();
}

bool BillboardMuzzleFlash::Initialize(Renderer* renderer, std::shared_ptr<Texture> texture, float size, float lifetime)
{
    BillboardObject::Initialize(renderer);
    SetTexture(texture);
    SetSize(size);
    SetLifetime(lifetime);
    EnableBillboarding(true);

    shaderInfo.vertexShaderName = L"FlashVertexShader";
    shaderInfo.pixelShaderName = L"FlashPixelShader";

    vertexShader = renderer->GetShaderManager()->GetVertexShader(shaderInfo.vertexShaderName);
    pixelShader = renderer->GetShaderManager()->GetPixelShader(shaderInfo.pixelShaderName);
    inputLayout = renderer->GetShaderManager()->GetInputLayout(shaderInfo.vertexShaderName);

    auto device = renderer->GetDevice();
    if (!CreateBlendState(device)) return false;
    if (!CreateRasterizerState(device)) return false;

    materialData.ambient = XMFLOAT3(0, 0, 0);
    materialData.diffuse = XMFLOAT3(1, 1, 1);
    materialData.specular = XMFLOAT3(0, 0, 0);
    materialData.alpha = 1.0f;
    materialData.useTexture = (texture != nullptr) ? 1 : 0;

    return true;
}

bool BillboardMuzzleFlash::CreateBlendState(ID3D11Device* device)
{
    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].BlendEnable = FALSE;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    return SUCCEEDED(device->CreateBlendState(&desc, blendState.GetAddressOf()));
}

bool BillboardMuzzleFlash::CreateRasterizerState(ID3D11Device* device)
{
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;

    return SUCCEEDED(device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf()));
}

void BillboardMuzzleFlash::SetFadeOut(bool enable)
{
    fadeOut = enable;
}

void BillboardMuzzleFlash::Update(float deltaTime)
{
    BillboardObject::Update(deltaTime);

    if (fadeOut && lifetime > 0.0f)
        materialData.alpha = std::max<float>(0.0f, 1.0f - (elapsedTime / lifetime));
    else
        materialData.alpha = 1.0f;
}

void BillboardMuzzleFlash::Render(Renderer* renderer)
{
    if (!IsAlive()) return;

    auto context = renderer->GetDeviceContext();

    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);

    ID3D11BlendState* prevBlend = nullptr;
    FLOAT blendFactor[4] = {};
    UINT mask = 0;
    context->OMGetBlendState(&prevBlend, blendFactor, &mask);
    context->OMSetBlendState(blendState.Get(), blendFactor, 0xffffffff);

    ID3D11RasterizerState* prevRasterizer = nullptr;
    context->RSGetState(&prevRasterizer);
    context->RSSetState(rasterizerState.Get());

    BillboardObject::Render(renderer);

    context->OMSetBlendState(prevBlend, blendFactor, mask);
    if (prevBlend) prevBlend->Release();

    context->RSSetState(prevRasterizer);
    if (prevRasterizer) prevRasterizer->Release();
}
