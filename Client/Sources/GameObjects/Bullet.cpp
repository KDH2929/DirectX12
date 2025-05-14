#include "Bullet.h"
#include "Renderer.h"
#include "PhysicsManager.h"
#include "MathUtil.h"

Bullet::Bullet(const XMFLOAT3& position, const XMFLOAT3& direction, float speed, std::shared_ptr<Mesh> mesh, std::shared_ptr<Texture> texture)
    : direction(direction), speed(speed), bulletMesh(mesh)
{
    this->position = position;
    this->diffuseTexture = texture;

    if (diffuseTexture == nullptr) {
        exit(-1);
    }
}

bool Bullet::Initialize(Renderer* renderer)
{
    GameObject::Initialize(renderer);

    shaderInfo.vertexShaderName = L"PhongVertexShader";
    shaderInfo.pixelShaderName = L"PhongPixelShader";

    auto device = renderer->GetDevice();
    auto shaderMgr = renderer->GetShaderManager();

    vertexShader = shaderMgr->GetVertexShader(shaderInfo.vertexShaderName);
    pixelShader = shaderMgr->GetPixelShader(shaderInfo.pixelShaderName);
    inputLayout = shaderMgr->GetInputLayout(shaderInfo.vertexShaderName);

    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.CullMode = shaderInfo.cullMode;
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = true;
    device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf());

    materialData.ambient = XMFLOAT3(0.1f, 0.1f, 0.1f);
    materialData.diffuse = XMFLOAT3(1.0f, 1.0f, 1.0f);
    materialData.specular = XMFLOAT3(0.5f, 0.5f, 0.5f);
    materialData.useTexture = (diffuseTexture ? 1 : 0);

    SetScale(XMFLOAT3(35.0f, 35.0f, 35.0f));

    // PhysX: ÅºÈ¯ »ý¼º
    PxRigidDynamic* actor = PhysicsManager::GetInstance()->CreateProjectile(
        PxVec3(position.x, position.y, position.z),
        PxVec3(direction.x, direction.y, direction.z),
        speed,
        1.0f,
        1.0f,
        CollisionLayer::Projectile
    );

    BindPhysicsActor(actor);
    actor->userData = this;

    return true;
}

void Bullet::Update(float deltaTime)
{
    if (!isAlive)
        return;

    SyncFromPhysics();
    UpdateWorldMatrix();
    DrawPhysicsColliderDebug();

    lifeTime += deltaTime;
    if (lifeTime > maxLifeTime)
    {
        Deactivate();
    }

}

void Bullet::Render(Renderer* renderer)
{
    if (!isAlive)
        return;

    auto device = renderer->GetDevice();
    auto context = renderer->GetDeviceContext();

    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);
    context->RSSetState(rasterizerState.Get());

    if (auto mesh = bulletMesh.lock())
    {
        CB_MVP mvpData;
        mvpData.model = XMMatrixTranspose(worldMatrix);
        mvpData.view = XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix());
        mvpData.projection = XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix());
        mvpData.modelInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));

        context->UpdateSubresource(constantMVPBuffer.Get(), 0, nullptr, &mvpData, 0, 0);
        context->VSSetConstantBuffers(0, 1, constantMVPBuffer.GetAddressOf());

        ID3D11Buffer* lightCB = renderer->GetLightingConstantBuffer();
        context->PSSetConstantBuffers(1, 1, &lightCB);

        context->UpdateSubresource(constantMaterialBuffer.Get(), 0, nullptr, &materialData, 0, 0);
        context->PSSetConstantBuffers(2, 1, constantMaterialBuffer.GetAddressOf());

        if (diffuseTexture) {
            ID3D11ShaderResourceView* srv = diffuseTexture->GetShaderResourceView();
            ID3D11SamplerState* sampler = renderer->GetSamplerState();
            context->PSSetShaderResources(0, 1, &srv);
            context->PSSetSamplers(0, 1, &sampler);
        }

        ID3D11Buffer* vb = mesh->GetVertexBuffer();
        ID3D11Buffer* ib = mesh->GetIndexBuffer();
        UINT stride = sizeof(MeshVertex);
        UINT offset = 0;

        context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        context->DrawIndexed(mesh->GetIndexCount(), 0, 0);
    }
}

void Bullet::SetMaxLifeTime(float seconds)
{
    maxLifeTime = seconds;
}

void Bullet::OnHit(GameObject* hitTarget)
{
    Deactivate();
}

void Bullet::Activate(const XMFLOAT3& position, const XMFLOAT3& direction, float speed)
{
    this->position = position;
    this->direction = direction;
    this->speed = speed;
    this->lifeTime = 0.0f;
    this->isAlive = true;

    if (physicsActor) {
        if (auto* dynamicActor = physicsActor->is<PxRigidDynamic>()) {
            dynamicActor->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, false);
            dynamicActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
            dynamicActor->setLinearVelocity(PxVec3(direction.x, direction.y, direction.z) * speed);
            dynamicActor->setGlobalPose(PxTransform(PxVec3(position.x, position.y, position.z)));
        }
    }
}

void Bullet::Deactivate()
{
    isAlive = false;

    if (physicsActor) {
        if (auto* dynamicActor = physicsActor->is<PxRigidDynamic>()) {
            dynamicActor->setLinearVelocity(PxVec3(0));
            dynamicActor->setAngularVelocity(PxVec3(0));
            dynamicActor->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, true);
        }
    }
}
