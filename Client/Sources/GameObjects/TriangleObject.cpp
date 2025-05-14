#include "TriangleObject.h"
#include "Renderer.h"
#include "InputManager.h"
#include "DebugManager.h"

#include <iostream>

TriangleObject::TriangleObject()
    : GameObject()
{
}

bool TriangleObject::Initialize(Renderer* renderer)
{
    GameObject::Initialize(renderer);

    CreateGeometry();

    auto device = renderer->GetDevice();
    auto shaderManager = renderer->GetShaderManager();

    shaderInfo.vertexShaderName = L"TriangleVertexShader";
    shaderInfo.pixelShaderName = L"TrianglePixelShader";

    vertexShader = shaderManager->GetVertexShader(shaderInfo.vertexShaderName);
    pixelShader = shaderManager->GetPixelShader(shaderInfo.pixelShaderName);
    inputLayout = shaderManager->GetInputLayout(shaderInfo.vertexShaderName);


    // Vertex Buffer 생성
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = UINT(sizeof(SimpleVertex) * vertices.size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vinitData = {};
    vinitData.pSysMem = vertices.data();

    HRESULT hr = device->CreateBuffer(&vbd, &vinitData, vertexBuffer.GetAddressOf());
    if (FAILED(hr))
        return false;

    // Index Buffer 생성
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = UINT(sizeof(UINT) * indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = indices.data();

    hr = device->CreateBuffer(&ibd, &iinitData, indexBuffer.GetAddressOf());
    if (FAILED(hr))
        return false;

    return true;
}

void TriangleObject::CreateGeometry()
{
    // 간단한 평면 삼각형 만들기
    vertices = {
        { XMFLOAT3(0.0f, 0.5f, 0.0f), XMFLOAT4(1,0,0,1) }, // top (빨간색)
        { XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT4(0,1,0,1) }, // right (초록색)
        { XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT4(0,0,1,1) } // left (파란색)
    };

    indices = { 0, 1, 2 };
}

void TriangleObject::Update(float deltaTime)
{
    // 삼각형 이동
    // Input Manager 를 통해 상하좌우 이동 시키기

    if (InputManager::GetInstance().IsKeyHeld(VK_LEFT)) {
        position.x -= 1.0f * deltaTime;
    }
    if (InputManager::GetInstance().IsKeyHeld(VK_RIGHT)) {
        position.x += 1.0f * deltaTime;
    }

    if (InputManager::GetInstance().IsKeyHeld(VK_UP)) {
        position.y += 1.0f * deltaTime;
    }
    if (InputManager::GetInstance().IsKeyHeld(VK_DOWN)) {
        position.y -= 1.0f * deltaTime;
    }

    UpdateWorldMatrix();
}

void TriangleObject::Render(Renderer* renderer) {

    auto context = renderer->GetDeviceContext();

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;

    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);


    XMMATRIX view = renderer->GetCamera()->GetViewMatrix();
    XMMATRIX proj = renderer->GetCamera()->GetProjectionMatrix();

    CB_MVP mvpData;
    mvpData.model = XMMatrixTranspose(worldMatrix);
    mvpData.view = XMMatrixTranspose(view);
    mvpData.projection = XMMatrixTranspose(proj);
    mvpData.modelInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));

    context->UpdateSubresource(constantMVPBuffer.Get(), 0, nullptr, &mvpData, 0, 0);
    context->VSSetConstantBuffers(0, 1, constantMVPBuffer.GetAddressOf());

    context->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
}
