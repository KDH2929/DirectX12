#include "BillboardExplosion.h"
#include "Renderer.h"
#include "DebugManager.h"

BillboardExplosion::BillboardExplosion() {}

BillboardExplosion::~BillboardExplosion()
{
    blendState.Reset();
    rasterizerState.Reset();
    uvRectConstantBuffer.Reset();
}

bool BillboardExplosion::Initialize(Renderer* renderer, std::shared_ptr<Texture> spriteSheet, int rows, int cols, float duration)
{
    BillboardObject::Initialize(renderer);
    SetTexture(spriteSheet);
    EnableBillboarding(true);
    SetLifetime(duration);

    this->rows = rows;
    this->cols = cols;
    this->totalFrames = rows * cols;
    this->frameDuration = duration / static_cast<float>(totalFrames);
    

    shaderInfo.vertexShaderName = L"FlashVertexShader";
    shaderInfo.pixelShaderName = L"ExplosionPixelShader";

    vertexShader = renderer->GetShaderManager()->GetVertexShader(shaderInfo.vertexShaderName);
    pixelShader = renderer->GetShaderManager()->GetPixelShader(shaderInfo.pixelShaderName);
    inputLayout = renderer->GetShaderManager()->GetInputLayout(shaderInfo.vertexShaderName);

    materialData.useTexture = (spriteSheet != nullptr) ? 1 : 0;
    materialData.alpha = 1.0f;

    auto device = renderer->GetDevice();
    if (!CreateBlendState(device)) return false;
    if (!CreateRasterizerState(device)) return false;

    // Create the UVRect constant buffer
    CreateUVRectBuffer(device);

    return true;
}

void BillboardExplosion::Update(float deltaTime)
{
    BillboardObject::Update(deltaTime);

    frameTime += deltaTime;
    if (frameTime >= frameDuration)
    {
        frameTime -= frameDuration;
        ++currentFrame;

        if (currentFrame >= totalFrames)
        {
            currentFrame = totalFrames - 1;
            materialData.alpha = 0.0f;
            return;
        }

        int frameX = currentFrame % cols;
        int frameY = currentFrame / cols;

        uvWidth = 1.0f / cols;
        uvHeight = 1.0f / rows;

        float u0 = frameX * uvWidth;
        float v0 = frameY * uvHeight;
        float u1 = (frameX + 1) * uvWidth;
        float v1 = (frameY + 1) * uvHeight;

        // uvRect[0] = ÁÂ»ó, uvRect[3] = ¿ìÇÏ
        uvRect[0] = { u0, v0 };  // ÁÂ»ó
        uvRect[1] = { u1, v0 };  // ¿ì»ó
        uvRect[2] = { u0, v1 };  // ÁÂÇÏ
        uvRect[3] = { u1, v1 };  // ¿ìÇÏ
    }
}

void BillboardExplosion::Render(Renderer* renderer)
{
    if (!IsAlive()) return;

    auto context = renderer->GetDeviceContext();

    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);
    context->RSSetState(rasterizerState.Get());

    // UV rect
    context->UpdateSubresource(uvRectConstantBuffer.Get(), 0, nullptr, &uvRect, 0, 0);
    context->PSSetConstantBuffers(4, 1, uvRectConstantBuffer.GetAddressOf());  // b4 for uvRect


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

void BillboardExplosion::Activate()
{
    currentFrame = 0;
    frameTime = 0.0f;
    materialData.alpha = 1.0f;

    ResetElapsedTime();
}

void BillboardExplosion::DeActivate()
{
    materialData.alpha = 0.0f;
    elapsedTime = lifetime + 1.0f;
}

void BillboardExplosion::CreateUVRectBuffer(ID3D11Device* device)
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(CB_UVRect);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    HRESULT hr = device->CreateBuffer(&cbDesc, nullptr, uvRectConstantBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create UVRect constant buffer!\n");
    }
}


bool BillboardExplosion::CreateBlendState(ID3D11Device* device)
{
    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].BlendEnable = FALSE;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    return SUCCEEDED(device->CreateBlendState(&desc, blendState.GetAddressOf()));
}

bool BillboardExplosion::CreateRasterizerState(ID3D11Device* device)
{
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;

    return SUCCEEDED(device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf()));
}
