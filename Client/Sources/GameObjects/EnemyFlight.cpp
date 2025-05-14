#include "EnemyFlight.h"
#include "DebugManager.h"

EnemyFlight::EnemyFlight(std::shared_ptr<Mesh> mesh, std::shared_ptr<Texture> texture)
    : Flight(mesh, texture)
{
    SetIsEnemy(true);
    SetHealth(100.0f);
}

bool EnemyFlight::Initialize(Renderer* renderer) {
    if (!Flight::Initialize(renderer))
        return false;

    SetAlive(true);

    XMFLOAT3 currentPosition = GetPosition();

    // Enemy 용 충돌체 생성
    PxRigidDynamic* actor = PhysicsManager::GetInstance()->CreateDynamicBox(
        { currentPosition.x, currentPosition.y, currentPosition.z },
        { 4, 4, 4 }, { 0, 0, 0 }, CollisionLayer::Enemy);

    // 적이 다른 오브젝트와 충돌할 수 있도록 응답 설정 (예: Default랑 Block)
    PhysicsManager::GetInstance()->SetCollisionResponse(
        CollisionLayer::Enemy, CollisionLayer::Default, CollisionResponse::Block);

    actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);

    // 물리 재질 설정
    PxMaterial* material = PhysicsManager::GetInstance()->GetDefaultMaterial();
    material->setRestitution(0.0f);
    material->setStaticFriction(0.0f);
    material->setDynamicFriction(0.0f);

    PxShape* shape;
    actor->getShapes(&shape, 1);
    shape->setMaterials(&material, 1);

    BindPhysicsActor(actor);

    if (auto* dynamicActor = physicsActor->is<PxRigidDynamic>()) {
        dynamicActor->setLinearVelocity(PxVec3(0, 0, 0));
        dynamicActor->setAngularVelocity(PxVec3(0, 0, 0));
    }

    SetColliderOffset(XMFLOAT3(0.0f, 5.0f, 0.0f));
    SetCollisionLayer(CollisionLayer::Enemy);

    return true;
}

void EnemyFlight::Update(float deltaTime) {

    if (!IsAlive()) return;

    Flight::Update(deltaTime);

    SyncFromPhysics();
    UpdateWorldMatrix();
    DrawPhysicsColliderDebug();
}

void EnemyFlight::SetExplosionEffect(std::shared_ptr<BillboardExplosion> explosion)
{
    explosionEffect = explosion;
}

void EnemyFlight::OnDestroyed()
{
    DebugManager::GetInstance().AddOnScreenMessage("EnemyFlight Destroyed!", 10.0f);

    SetAlive(false);

    // 씬에서 제거
    if (physicsActor) {
        PxScene* scene = PhysicsManager::GetInstance()->GetScene();

        PhysicsManager::GetInstance()->RemoveActor(physicsActor);

        physicsActor = nullptr;  // 내부 포인터 제거
    }


    if (explosionEffect)
    {
        explosionEffect->SetPosition(GetPosition());
        explosionEffect->Activate(); // 프레임/시간 초기화 및 알파 1.0 등
    }
}
