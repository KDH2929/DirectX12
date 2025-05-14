#include "PhysicsManager.h"
#include <Windows.h>

PhysicsManager* PhysicsManager::s_instance = nullptr;

PhysicsManager* PhysicsManager::GetInstance() {
    return s_instance;
}

void PhysicsManager::SetInstance(PhysicsManager* instance) {
    s_instance = instance;
}

bool PhysicsManager::Init()
{
    foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
    if (!foundation) return false;

    pvd = PxCreatePvd(*foundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PHYSX_PVD_HOST, PHYSX_PVD_PORT, PHYSX_PVD_TIMEOUT);
    pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true, pvd);
    if (!physics) return false;

    PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, PHYSX_GRAVITY_Y, 0.0f);
    dispatcher = PxDefaultCpuDispatcherCreate(PHYSX_NUM_THREADS);
    sceneDesc.cpuDispatcher = dispatcher;
    sceneDesc.filterShader = &PhysicsManager::CustomFilterShader;
    sceneDesc.simulationEventCallback = this;

    scene = physics->createScene(sceneDesc);
    if (!scene) return false;

    defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.6f);

    // Default 모든 충돌 Block으로 초기화
    for (int i = 0; i < 32; ++i)
        for (int j = 0; j < 32; ++j)
            collisionMatrix[i][j] = CollisionResponse::Block;

    return true;
}

void PhysicsManager::Cleanup()
{
    for (auto actor : managedActors) {
        if (actor) actor->release();
    }
    managedActors.clear();

    if (scene) scene->release();
    if (dispatcher) dispatcher->release();
    if (defaultMaterial) defaultMaterial->release();
    if (physics) physics->release();
    if (pvd) {
        PxPvdTransport* transport = pvd->getTransport();
        pvd->release();
        if (transport) transport->release();
    }
    if (foundation) foundation->release();
}

void PhysicsManager::Simulate(float deltaTime)
{
    if (scene) scene->simulate(deltaTime);
}

void PhysicsManager::FetchResults()
{
    if (scene) scene->fetchResults(true);
}

void PhysicsManager::SetCollisionResponse(CollisionLayer a, CollisionLayer b, CollisionResponse response)
{
    collisionMatrix[uint32_t(a)][uint32_t(b)] = response;
    collisionMatrix[uint32_t(b)][uint32_t(a)] = response;
}

void PhysicsManager::RegisterHitHandler(CollisionEventHandler handler)
{
    hitHandlers.push_back(std::move(handler));
}

void PhysicsManager::RegisterOverlapHandler(CollisionEventHandler handler)
{
    overlapHandlers.push_back(std::move(handler));
}

void PhysicsManager::RemoveActor(PxActor* actor)
{
    if (actor) {
        actorsToRemove.push_back(actor);
    }
}

void PhysicsManager::ApplyActorRemovals()
{
    for (PxActor* actor : actorsToRemove) {
        if (scene && actor) {
            scene->removeActor(*actor);
        }
    }
    actorsToRemove.clear();
}


void PhysicsManager::SetShapeCollisionFilter(PxShape* shape, CollisionLayer layer)
{
    PxFilterData filterData;
    filterData.word0 = uint32_t(layer);   // 레이어 등록
    shape->setSimulationFilterData(filterData);
}

PxFilterFlags PhysicsManager::CustomFilterShader(
    PxFilterObjectAttributes attr0, PxFilterData data0,
    PxFilterObjectAttributes attr1, PxFilterData data1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32)
{
    auto* manager = PhysicsManager::GetInstance();
    if (!manager) return PxFilterFlag::eSUPPRESS;

    if (PxFilterObjectIsTrigger(attr0) || PxFilterObjectIsTrigger(attr1)) {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }

    CollisionLayer l0 = static_cast<CollisionLayer>(data0.word0);
    CollisionLayer l1 = static_cast<CollisionLayer>(data1.word0);
    CollisionResponse response = manager->collisionMatrix[int(l0)][int(l1)];

    switch (response)
    {
    case CollisionResponse::Block:
        pairFlags = PxPairFlag::eCONTACT_DEFAULT
            | PxPairFlag::eNOTIFY_TOUCH_FOUND
            | PxPairFlag::eNOTIFY_TOUCH_LOST;
        return PxFilterFlag::eDEFAULT;

    case CollisionResponse::Overlap:
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;

    case CollisionResponse::Ignore:
        return PxFilterFlag::eSUPPRESS;

    default:
        return PxFilterFlag::eDEFAULT;
    }
}

void PhysicsManager::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
    PxActor* actorA = pairHeader.actors[0];
    PxActor* actorB = pairHeader.actors[1];

    if (!actorA->userData || !actorB->userData) return;

    for (auto& handler : hitHandlers) {
        if (handler.Matches(actorA, actorB)) {
            handler.Callback(actorA, actorB);
        }
    }
}

void PhysicsManager::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
    for (PxU32 i = 0; i < count; ++i)
    {
        PxTriggerPair& pair = pairs[i];

        // 둘 다 유효한 경우만 처리
        if (!pair.triggerActor || !pair.otherActor)
            continue;

        // Overlap 이벤트만 필터링
        if (pair.status & PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            for (auto& handler : overlapHandlers)
            {
                if (handler.Matches(pair.triggerActor, pair.otherActor))
                {
                    handler.Callback(pair.triggerActor, pair.otherActor);
                }
            }
        }
    }
}

PxRigidStatic* PhysicsManager::CreateStaticBox(const PxVec3& pos, const PxVec3& halfExtents, CollisionLayer layer)
{
    PxRigidStatic* actor = physics->createRigidStatic(PxTransform(pos));
    PxShape* shape = physics->createShape(PxBoxGeometry(halfExtents), *defaultMaterial);
    SetShapeCollisionFilter(shape, layer);
    actor->attachShape(*shape);
    shape->release();
    scene->addActor(*actor);
    managedActors.push_back(actor);
    return actor;
}

PxRigidDynamic* PhysicsManager::CreateDynamicBox(const PxVec3& pos, const PxVec3& halfExtents, const PxVec3& velocity, CollisionLayer layer)
{
    PxRigidDynamic* actor = physics->createRigidDynamic(PxTransform(pos));
    PxShape* shape = physics->createShape(PxBoxGeometry(halfExtents), *defaultMaterial);
    SetShapeCollisionFilter(shape, layer);
    actor->attachShape(*shape);
    shape->release();
    actor->setLinearVelocity(velocity);
    scene->addActor(*actor);
    managedActors.push_back(actor);
    return actor;
}

PxRigidDynamic* PhysicsManager::CreateSphere(const PxVec3& pos, float radius, const PxVec3& velocity, CollisionLayer layer)
{
    PxRigidDynamic* actor = physics->createRigidDynamic(PxTransform(pos));
    PxShape* shape = physics->createShape(PxSphereGeometry(radius), *defaultMaterial);
    SetShapeCollisionFilter(shape, layer);
    actor->attachShape(*shape);
    shape->release();
    actor->setLinearVelocity(velocity);
    scene->addActor(*actor);
    managedActors.push_back(actor);
    return actor;
}

PxRigidDynamic* PhysicsManager::CreateCapsule(const PxVec3& pos, float radius, float halfHeight, const PxVec3& velocity, CollisionLayer layer)
{
    PxRigidDynamic* actor = physics->createRigidDynamic(PxTransform(pos));
    PxShape* shape = physics->createShape(PxCapsuleGeometry(radius, halfHeight), *defaultMaterial);
    SetShapeCollisionFilter(shape, layer);
    actor->attachShape(*shape);
    shape->release();
    actor->setLinearVelocity(velocity);
    scene->addActor(*actor);
    managedActors.push_back(actor);
    return actor;
}

PxRigidDynamic* PhysicsManager::CreateProjectile(const PxVec3& pos, const PxVec3& dir, float speed, float radius, float mass, CollisionLayer layer)
{
    PxRigidDynamic* actor = physics->createRigidDynamic(PxTransform(pos));
    PxShape* shape = physics->createShape(PxSphereGeometry(radius), *defaultMaterial);
    SetShapeCollisionFilter(shape, layer);
    actor->attachShape(*shape);
    shape->release();
    actor->setLinearVelocity(dir.getNormalized() * speed);
    actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
    actor->setMass(mass);
    scene->addActor(*actor);
    managedActors.push_back(actor);
    return actor;
}
