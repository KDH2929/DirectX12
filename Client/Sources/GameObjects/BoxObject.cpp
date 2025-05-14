#include "BoxObject.h"
#include "Renderer.h"
#include "ShaderManager.h"
#include "Mesh.h"

BoxObject::BoxObject() {
    position = XMFLOAT3(0, 0, 0);
    scale = XMFLOAT3(3.0f, 3.0f, 3.0f);
    rotationEuler = XMFLOAT3(0, 0, 0);
    rotationQuat = XMQuaternionIdentity();
}

bool BoxObject::Initialize(Renderer* renderer) {
    GameObject::Initialize(renderer);

    // 큐브 메시 생성
    cubeMesh = Mesh::CreateCube(renderer->GetDevice());
    if (!cubeMesh) return false;

    auto device = renderer->GetDevice();
    auto shaderManager = renderer->GetShaderManager();

    // Phong 셰이더 설정
    shaderInfo.vertexShaderName = L"PhongVertexShader";
    shaderInfo.pixelShaderName = L"PhongPixelShader";

    vertexShader = shaderManager->GetVertexShader(shaderInfo.vertexShaderName);
    pixelShader = shaderManager->GetPixelShader(shaderInfo.pixelShaderName);
    inputLayout = shaderManager->GetInputLayout(shaderInfo.vertexShaderName);

    // 래스터라이저 설정
    shaderInfo.cullMode = D3D11_CULL_BACK;
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.CullMode = shaderInfo.cullMode;
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = true;
    device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf());

    // 머티리얼: 텍스처 없이 빨간색
    materialData.ambient = XMFLOAT3(0.2f, 0.0f, 0.0f);
    materialData.diffuse = XMFLOAT3(1.0f, 0.0f, 0.0f); // 진한 빨강
    materialData.specular = XMFLOAT3(0.4f, 0.4f, 0.4f);
    materialData.useTexture = 0;

    return true;
}

void BoxObject::Update(float deltaTime) {
    UpdateWorldMatrix();
}

void BoxObject::Render(Renderer* renderer) {
    auto context = renderer->GetDeviceContext();
    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);
    context->RSSetState(rasterizerState.Get());

    if (cubeMesh) {
        // CB_MVP
        CB_MVP mvpData;
        mvpData.model = XMMatrixTranspose(worldMatrix);
        mvpData.view = XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix());
        mvpData.projection = XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix());
        mvpData.modelInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));

        context->UpdateSubresource(constantMVPBuffer.Get(), 0, nullptr, &mvpData, 0, 0);
        context->VSSetConstantBuffers(0, 1, constantMVPBuffer.GetAddressOf());

        // 조명: b1
        ID3D11Buffer* lightCB = renderer->GetLightingConstantBuffer();
        context->PSSetConstantBuffers(1, 1, &lightCB);

        // 머티리얼: b2
        context->UpdateSubresource(constantMaterialBuffer.Get(), 0, nullptr, &materialData, 0, 0);
        context->PSSetConstantBuffers(2, 1, constantMaterialBuffer.GetAddressOf());

        // 메쉬 버퍼 바인딩
        UINT stride = sizeof(MeshVertex);
        UINT offset = 0;
        ID3D11Buffer* vb = cubeMesh->GetVertexBuffer();
        ID3D11Buffer* ib = cubeMesh->GetIndexBuffer();

        context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        context->DrawIndexed(cubeMesh->GetIndexCount(), 0, 0);
    }
}
