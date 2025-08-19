#pragma once

#include "physx/PxPhysicsAPI.h"
#include <vector>
#include <unordered_map>
#include <functional>

using namespace physx;

// 설정 상수
constexpr const char* PHYSX_PVD_HOST = "127.0.0.1";
constexpr int         PHYSX_PVD_PORT = 5425;
constexpr int         PHYSX_PVD_TIMEOUT = 10;
constexpr int         PHYSX_NUM_THREADS = 2;
constexpr float       PHYSX_GRAVITY_Y = -9.81f;

// 충돌 응답 타입
enum class CollisionResponse
{
    Ignore,
    Overlap,
    Block
};

// 콜리전 레이어 정의
enum class CollisionLayer : uint32_t
{
    Default = 1,
    Player = 2,
    Enemy = 3,
    Projectile = 4,
    EnemyProjectile = 5,
    PlayerProjectile = 6,
};


struct CollisionEventHandler
{
    // 필터링 조건 (예: 특정 레이어 쌍, 오브젝트 타입 등)
    std::function<bool(PxActor*, PxActor*)> Matches;

    // 처리할 콜백
    std::function<void(PxActor*, PxActor*)> Callback;
};

class PhysicsManager : public PxSimulationEventCallback
{
public:
    // 전역 인스턴스 접근
    static PhysicsManager* GetInstance();
    static void SetInstance(PhysicsManager* instance);

    // 생성 & 소멸
    bool Init();
    void Cleanup();

    void Simulate(float deltaTime);
    void FetchResults();

    // 접근자
    PxPhysics* GetPhysics() const { return physics; }
    PxScene* GetScene() const { return scene; }
    PxMaterial* GetDefaultMaterial() const { return defaultMaterial; }

    // 오브젝트 생성
    PxRigidStatic* CreateStaticBox(const PxVec3& pos, const PxVec3& halfExtents, CollisionLayer layer = CollisionLayer::Default);
    PxRigidDynamic* CreateDynamicBox(const PxVec3& pos, const PxVec3& halfExtents, const PxVec3& velocity, CollisionLayer layer = CollisionLayer::Default);
    PxRigidDynamic* CreateSphere(const PxVec3& pos, float radius, const PxVec3& velocity, CollisionLayer layer = CollisionLayer::Default);
    PxRigidDynamic* CreateCapsule(const PxVec3& pos, float radius, float halfHeight, const PxVec3& velocity, CollisionLayer layer = CollisionLayer::Default);
    PxRigidDynamic* CreateProjectile(const PxVec3& pos, const PxVec3& dir, float speed, float radius, float mass = 1.0f, CollisionLayer layer = CollisionLayer::Projectile);

    // 충돌 응답 설정 (Block, Overlap, Ignore)
    void SetCollisionResponse(CollisionLayer a, CollisionLayer b, CollisionResponse response);


    void RegisterHitHandler(CollisionEventHandler handler);
    void RegisterOverlapHandler(CollisionEventHandler handler);

    
    // 제거관련함수
    void RemoveActor(PxActor* actor);
    void ApplyActorRemovals();

private:
    // 내부 유틸
    void SetShapeCollisionFilter(PxShape* shape, CollisionLayer layer);

    // 충돌 필터 셰이더
    static PxFilterFlags CustomFilterShader(
        PxFilterObjectAttributes attributes0, PxFilterData filterData0,
        PxFilterObjectAttributes attributes1, PxFilterData filterData1,
        PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize
    );

    // PxSimulationEventCallback 구현
    void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
    void onTrigger(PxTriggerPair* pairs, PxU32 count) override;
    void onConstraintBreak(PxConstraintInfo*, PxU32) override {}
    void onWake(PxActor**, PxU32) override {}
    void onSleep(PxActor**, PxU32) override {}
    void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) override {}

private:
    // 전역 인스턴스
    static PhysicsManager* s_instance;

    // PhysX 구성 요소
    PxDefaultAllocator      allocator;
    PxDefaultErrorCallback  errorCallback;
    PxFoundation* foundation = nullptr;
    PxPhysics* physics = nullptr;
    PxDefaultCpuDispatcher* dispatcher = nullptr;
    PxScene* scene = nullptr;
    PxMaterial* defaultMaterial = nullptr;
    PxPvd* pvd = nullptr;


    std::vector<PxActor*> actorsToRemove;


    // 충돌 응답 매트릭스
    CollisionResponse collisionMatrix[32][32] = {};

    // 리소스 관리
    std::vector<PxActor*> managedActors;

    // 이벤트 핸들러들 관리
    std::vector<CollisionEventHandler> hitHandlers;
    std::vector<CollisionEventHandler> overlapHandlers;

};
