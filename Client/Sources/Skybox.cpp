#include "Skybox.h"
#include "Renderer.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "Camera.h"
#include "Mesh.h"

#include <DirectXMath.h>
#include <DirectXTK/DDSTextureLoader.h>

using namespace DirectX;

Skybox::Skybox(const std::wstring& path)
    : cubeMapPath(path) {
    scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
    position = XMFLOAT3(0, 0, 0);
}

bool Skybox::Initialize(Renderer* renderer) {
    GameObject::Initialize(renderer);

    auto device = renderer->GetDevice();
    auto context = renderer->GetDeviceContext();
    auto shaderManager = renderer->GetShaderManager();

    cubeMesh = Mesh::CreateCube(device);

    shaderInfo.vertexShaderName = L"SkyboxVertexShader";
    shaderInfo.pixelShaderName = L"SkyboxPixelShader";
    vertexShader = shaderManager->GetVertexShader(shaderInfo.vertexShaderName);
    pixelShader = shaderManager->GetPixelShader(shaderInfo.pixelShaderName);
    inputLayout = shaderManager->GetInputLayout(shaderInfo.vertexShaderName);

    Microsoft::WRL::ComPtr<ID3D11Resource> textureResource;
    HRESULT hr = DirectX::CreateDDSTextureFromFileEx(
        device, cubeMapPath.c_str(), 0,
        D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0,
        D3D11_RESOURCE_MISC_TEXTURECUBE,
        static_cast<DDS_LOADER_FLAGS>(0),
        textureResource.GetAddressOf(), cubeTextureSRV.GetAddressOf());

    if (FAILED(hr)) {
        OutputDebugStringW(L"Skybox DDS 텍스처 로딩 실패\n");
        return false;
    }

    SetDepthState(device);
    SetRasterizerState(device);
    SetScale(XMFLOAT3(100.0f, 100.0f, 100.0f));
    // 스케일을 너무 키우면 카메라 절두체 바깥을 나가버림 (주의할 것)

    return true;
}

void Skybox::SetDepthState(ID3D11Device* device) {
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = true;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    device->CreateDepthStencilState(&desc, depthState.GetAddressOf());
}

void Skybox::SetRasterizerState(ID3D11Device* device) {
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE; // Cull 없이 양면 렌더링
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = true;
    device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf());
}

void Skybox::Update(float deltaTime) {}

void Skybox::Render(Renderer* renderer) {
    auto context = renderer->GetDeviceContext();
    auto camera = renderer->GetCamera();

    ID3D11DepthStencilState* prevDepthState = nullptr;
    UINT prevStencilRef = 0;
    context->OMGetDepthStencilState(&prevDepthState, &prevStencilRef);
    context->OMSetDepthStencilState(depthState.Get(), 0);

    ID3D11RasterizerState* prevRasterizer = nullptr;
    context->RSGetState(&prevRasterizer);
    context->RSSetState(rasterizerState.Get());

    XMMATRIX view = camera->GetViewMatrix();
    view.r[3] = XMVectorSet(0, 0, 0, 1);

    XMMATRIX proj = camera->GetProjectionMatrix();
    XMMATRIX model = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

    CB_MVP mvpData;
    mvpData.model = XMMatrixTranspose(model);
    mvpData.view = XMMatrixTranspose(view);
    mvpData.projection = XMMatrixTranspose(proj);
    mvpData.modelInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, model));

    context->UpdateSubresource(constantMVPBuffer.Get(), 0, nullptr, &mvpData, 0, 0);
    context->VSSetConstantBuffers(0, 1, constantMVPBuffer.GetAddressOf());

    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);

    ID3D11SamplerState* sampler = renderer->GetSamplerState();
    context->PSSetShaderResources(0, 1, cubeTextureSRV.GetAddressOf());
    context->PSSetSamplers(0, 1, &sampler);

    UINT stride = sizeof(MeshVertex);
    UINT offset = 0;
    ID3D11Buffer* vb = cubeMesh->GetVertexBuffer();
    ID3D11Buffer* ib = cubeMesh->GetIndexBuffer();
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->DrawIndexed(cubeMesh->GetIndexCount(), 0, 0);

    context->OMSetDepthStencilState(prevDepthState, prevStencilRef);
    if (prevDepthState) prevDepthState->Release();

    context->RSSetState(prevRasterizer);
    if (prevRasterizer) prevRasterizer->Release();
}
