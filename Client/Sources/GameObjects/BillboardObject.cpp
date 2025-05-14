#include "BillboardObject.h"
#include "Renderer.h"

BillboardObject::BillboardObject() {}

bool BillboardObject::Initialize(Renderer* renderer)
{
    GameObject::Initialize(renderer);
    quadMesh = CreateQuadMesh(renderer->GetDevice());
    return quadMesh != nullptr;
}

void BillboardObject::SetTexture(std::shared_ptr<Texture> tex)
{
    texture = tex;
}

void BillboardObject::SetSize(float size)
{
    this->size = size;
    this->scale = XMFLOAT3(size, size, size);
    UpdateWorldMatrix();
}

void BillboardObject::EnableBillboarding(bool enable)
{
    enableBillboarding = enable;
}

bool BillboardObject::IsBillboardingEnabled() const
{
    return enableBillboarding;
}

void BillboardObject::SetLifetime(float seconds)
{
    lifetime = seconds;
    elapsedTime = 0.0f;
}

void BillboardObject::ResetElapsedTime()
{
    elapsedTime = 0.0f;
}

bool BillboardObject::IsAlive() const
{
    return (lifetime < 0.0f) || (elapsedTime < lifetime);
}

void BillboardObject::Update(float deltaTime)
{
    elapsedTime += deltaTime;
    UpdateWorldMatrix();
}

void BillboardObject::Render(Renderer* renderer)
{
    if (!texture || !quadMesh) return;

    auto context = renderer->GetDeviceContext();
    auto view = renderer->GetCamera()->GetViewMatrix();
    auto proj = renderer->GetCamera()->GetProjectionMatrix();

    if (enableBillboarding)
    {
        XMMATRIX invView = XMMatrixInverse(nullptr, view);
        XMVECTOR camRight = invView.r[0];
        XMVECTOR camUp = invView.r[1];
        XMVECTOR camForward = XMVector3Normalize(XMVector3Cross(camUp, camRight));

        XMMATRIX R;
        R.r[0] = camRight;
        R.r[1] = camUp;
        R.r[2] = camForward;
        R.r[3] = XMVectorSet(0, 0, 0, 1);

        XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
        XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);
        worldMatrix = S * R * T;
    }

    // MVP 상수 버퍼 업데이트
    CB_MVP mvpData;
    mvpData.model = XMMatrixTranspose(worldMatrix);
    mvpData.view = XMMatrixTranspose(view);
    mvpData.projection = XMMatrixTranspose(proj);
    mvpData.modelInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));
    context->UpdateSubresource(constantMVPBuffer.Get(), 0, nullptr, &mvpData, 0, 0);
    context->VSSetConstantBuffers(0, 1, constantMVPBuffer.GetAddressOf());

    // Material 상수 버퍼 업데이트
    context->UpdateSubresource(constantMaterialBuffer.Get(), 0, nullptr, &materialData, 0, 0);
    context->PSSetConstantBuffers(2, 1, constantMaterialBuffer.GetAddressOf());

    // 텍스처 바인딩
    ID3D11ShaderResourceView* srv = texture->GetShaderResourceView();
    if (srv)
    {
        ID3D11SamplerState* sampler = renderer->GetSamplerState();
        context->PSSetShaderResources(0, 1, &srv);
        context->PSSetSamplers(0, 1, &sampler);
    }

    // vb, ib 설정 및 드로우
    ID3D11Buffer* vb = quadMesh->GetVertexBuffer();
    ID3D11Buffer* ib = quadMesh->GetIndexBuffer();
    UINT stride = sizeof(MeshVertex);
    UINT offset = 0;

    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->DrawIndexed(quadMesh->GetIndexCount(), 0, 0);
}

std::shared_ptr<Mesh> BillboardObject::CreateQuadMesh(ID3D11Device* device)
{
    std::vector<MeshVertex> vertices = {
        {{-1, -1, 0}, {0, 0, -1}, {0, 1}},
        {{ 1, -1, 0}, {0, 0, -1}, {1, 1}},
        {{-1,  1, 0}, {0, 0, -1}, {0, 0}},
        {{ 1,  1, 0}, {0, 0, -1}, {1, 0}},
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 1, 3
    };

    auto mesh = std::make_shared<Mesh>();
    if (mesh->Initialize(device, vertices, indices)) {
        return mesh;
    }
    return nullptr;
}
