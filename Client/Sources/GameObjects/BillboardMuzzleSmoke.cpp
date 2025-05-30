#include "BillboardMuzzleSmoke.h"
#include "Renderer.h"

BillboardMuzzleSmoke::BillboardMuzzleSmoke() {}

BillboardMuzzleSmoke::~BillboardMuzzleSmoke()
{
    blendState.Reset();
    rasterizerState.Reset();
}

bool BillboardMuzzleSmoke::Initialize(Renderer* renderer, std::shared_ptr<Texture> mainTexture, std::shared_ptr<Texture> noiseTex, float size, float lifetime)
{
    BillboardObject::Initialize(renderer);
    SetTexture(mainTexture);
    noiseTexture = noiseTex;
    SetSize(size);
    SetLifetime(lifetime);
    EnableBillboarding(true);

    shaderInfo.vertexShaderName = L"SmokeVertexShader";
    shaderInfo.pixelShaderName = L"SmokePixelShader";

    vertexShader = renderer->GetShaderManager()->GetVertexShader(shaderInfo.vertexShaderName);
    pixelShader = renderer->GetShaderManager()->GetPixelShader(shaderInfo.pixelShaderName);
    inputLayout = renderer->GetShaderManager()->GetInputLayout(shaderInfo.vertexShaderName);

    auto device = renderer->GetDevice();
    if (!CreateBlendState(device)) return false;
    if (!CreateRasterizerState(device)) return false;
    if (!CreateDepthState(device)) return false;

    materialData.ambient = XMFLOAT3(0, 0, 0);
    materialData.diffuse = XMFLOAT3(1, 1, 1);
    materialData.specular = XMFLOAT3(0, 0, 0);
    materialData.alpha = 0.5f;
    materialData.useTexture = (mainTexture != nullptr) ? 1 : 0;

    return true;
}

bool BillboardMuzzleSmoke::CreateBlendState(ID3D11Device* device)
{
    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    return SUCCEEDED(device->CreateBlendState(&desc, blendState.GetAddressOf()));
}

bool BillboardMuzzleSmoke::CreateRasterizerState(ID3D11Device* device)
{
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;

    return SUCCEEDED(device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf()));
}

bool BillboardMuzzleSmoke::CreateDepthState(ID3D11Device* device)
{
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;          // 깊이 테스트 수행
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 깊이 기록 안 함
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    dsDesc.StencilEnable = FALSE;

    return SUCCEEDED(device->CreateDepthStencilState(&dsDesc, depthStencilState.GetAddressOf()));
}

void BillboardMuzzleSmoke::SetFadeOut(bool enable)
{
    fadeOut = enable;
}

void BillboardMuzzleSmoke::Update(float deltaTime)
{
    BillboardObject::Update(deltaTime);

    if (fadeOut && lifetime > 0.0f)
        materialData.alpha = std::max<float>(0.0f, 0.8f - (elapsedTime / lifetime));
    else
        materialData.alpha = 0.8f;
}

void BillboardMuzzleSmoke::Render(Renderer* renderer)
{
    if (!IsAlive()) return;

    auto context = renderer->GetDeviceContext();

    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);
    context->RSSetState(rasterizerState.Get());

    ID3D11BlendState* prevBlend = nullptr;
    FLOAT blendFactor[4] = {};
    UINT mask = 0;
    context->OMGetBlendState(&prevBlend, blendFactor, &mask);
    context->OMSetBlendState(blendState.Get(), blendFactor, 0xffffffff);

    ID3D11DepthStencilState* prevDepthState = nullptr;
    UINT stencilRef = 0;
    context->OMGetDepthStencilState(&prevDepthState, &stencilRef);
    context->OMSetDepthStencilState(depthStencilState.Get(), 0);

    if (noiseTexture)
    {
        ID3D11ShaderResourceView* srv = noiseTexture->GetShaderResourceView();
        context->PSSetShaderResources(1, 1, &srv);
    }

    BillboardObject::Render(renderer);


    context->OMSetBlendState(prevBlend, blendFactor, mask);
    if (prevBlend) prevBlend->Release();

    context->OMSetDepthStencilState(prevDepthState, stencilRef);
    if (prevDepthState) prevDepthState->Release();
}
