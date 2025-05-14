#include "Flight.h"
#include "Renderer.h"
#include "InputManager.h"
#include "DebugManager.h"


Flight::Flight(std::shared_ptr<Mesh> mesh, std::shared_ptr<Texture> texture)
    : flightMesh(mesh) {
    diffuseTexture = texture;
}

bool Flight::Initialize(Renderer* renderer)
{
    GameObject::Initialize(renderer);

    // Shader 초기화
    auto device = renderer->GetDevice();
    auto context = renderer->GetDeviceContext();
    auto shaderManager = renderer->GetShaderManager();

    shaderInfo.vertexShaderName = L"PhongVertexShader";
    shaderInfo.pixelShaderName = L"PhongPixelShader";

    vertexShader = shaderManager->GetVertexShader(shaderInfo.vertexShaderName);
    pixelShader = shaderManager->GetPixelShader(shaderInfo.pixelShaderName);
    inputLayout = shaderManager->GetInputLayout(shaderInfo.vertexShaderName);


    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.CullMode = shaderInfo.cullMode;
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = true;

    device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf());


    // 초기값 설정
    materialData.ambient = XMFLOAT3(0.1f, 0.1f, 0.1f);
    materialData.diffuse = XMFLOAT3(0.6f, 0.6f, 0.6f);
    materialData.specular = XMFLOAT3(0.3f, 0.3f, 0.3f);

    materialData.useTexture = (diffuseTexture != nullptr) ? 1 : 0;

    return true;
}

void Flight::Update(float deltaTime) {
}

void Flight::Render(Renderer* renderer) {

    if (!IsAlive())
        return;

    auto device = renderer->GetDevice();
    auto context = renderer->GetDeviceContext();
    

    // VS/PS/InputLayout 설정
    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);

    context->RSSetState(rasterizerState.Get());

    // Vertex/Index Buffer
    UINT stride = sizeof(MeshVertex);
    UINT offset = 0;

    if (auto mesh = flightMesh.lock()) {

        // CB_MVP 구성
        CB_MVP mvpData;
        mvpData.model = XMMatrixTranspose(worldMatrix);

        XMMATRIX view = renderer->GetCamera()->GetViewMatrix();
        XMMATRIX proj = renderer->GetCamera()->GetProjectionMatrix();

        mvpData.view = XMMatrixTranspose(view);
        mvpData.projection = XMMatrixTranspose(proj);
        mvpData.modelInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));

        context->UpdateSubresource(constantMVPBuffer.Get(), 0, nullptr, &mvpData, 0, 0);
        context->VSSetConstantBuffers(0, 1, constantMVPBuffer.GetAddressOf());  // b0: MVP + ModelInvTranspose

        // 조명: b1
        ID3D11Buffer* lightCB = renderer->GetLightingConstantBuffer();
        context->PSSetConstantBuffers(1, 1, &lightCB);

        // 머티리얼: b2
        context->UpdateSubresource(constantMaterialBuffer.Get(), 0, nullptr, &materialData, 0, 0);
        context->PSSetConstantBuffers(2, 1, constantMaterialBuffer.GetAddressOf());

        // 텍스처, 샘플러
        if (diffuseTexture) {
            ID3D11ShaderResourceView* srv = diffuseTexture->GetShaderResourceView();
            if (srv) {
                ID3D11SamplerState* sampler = renderer->GetSamplerState();
                context->PSSetShaderResources(0, 1, &srv);
                context->PSSetSamplers(0, 1, &sampler);
            }
        }

        // 메쉬 버퍼 바인딩
        ID3D11Buffer* vb = mesh->GetVertexBuffer();
        ID3D11Buffer* ib = mesh->GetIndexBuffer();
        context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        context->DrawIndexed(mesh->GetIndexCount(), 0, 0);
    }
    
}

void Flight::SetHealth(float hp)
{
    health = maxHealth = hp;
}

float Flight::GetHealth() const
{
    return health;
}

void Flight::TakeDamage(float damage)
{
    DebugManager::GetInstance().AddOnScreenMessage("Flight is damaged", 1.0f);

    health -= damage;
    if (health <= 0.0f && isAlive) {
        health = 0.0f;
        OnDestroyed();
    }
}

void Flight::OnDestroyed()
{
    DebugManager::GetInstance().AddOnScreenMessage("Flight Destroyed!", 10.0f);

    SetAlive(false);
}

void Flight::SetAlive(bool alive)
{
    isAlive = alive;
}

bool Flight::IsAlive() const
{
    return isAlive;
}

void Flight::SetIsEnemy(bool enemy)
{
    isEnemy = enemy;
}

bool Flight::IsEnemy() const
{
    return isEnemy;
}
