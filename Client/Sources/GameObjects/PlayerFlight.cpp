#include "PlayerFlight.h"
#include "Renderer.h"
#include "DebugManager.h"
#include "MathUtil.h"

#include <sstream>  
#include <algorithm>
#include <cmath>

PlayerFlight::PlayerFlight(std::shared_ptr<Mesh> mesh, std::shared_ptr<Texture> texture)
    : Flight(mesh, texture)
{
}
bool PlayerFlight::Initialize(Renderer* renderer)
{
    Flight::Initialize(renderer);

    // 카메라 초기화
    camera = std::make_shared<Camera>();

    XMFLOAT3 modelPosition = GetPosition();
    camera->SetPosition(XMFLOAT3(modelPosition.x + -2.0f, modelPosition.y + 4.0f, modelPosition.z - 18.0f));

    XMFLOAT3 initialEuler = { 0.0f, 0.0f, 0.0f }; // pitch, yaw, roll
    XMVECTOR initialQuat = XMQuaternionRotationRollPitchYaw(initialEuler.x, initialEuler.y, initialEuler.z);
    camera->SetRotationQuat(initialQuat);

    // perspective 설정
    float aspectRatio = static_cast<float>(renderer->GetViewportWidth()) / renderer->GetViewportHeight();
    camera->SetPerspective(XM_PIDIV4, aspectRatio, 0.1f, 2000.0f);

    camera->UpdateViewMatrix();

    cameraController.SetCameraMode(CameraMode::FlightThirdPerson);
    cameraController.SetCameraOffset(40.0f, 10.0f);

    // 마우스 커서를 숨기기
    InputManager::GetInstance().SetCursorHidden(true);

    auto muzzleFlashTexture = textureManager.LoadTexture(L"Assets/Textures/MuzzleFlash2.png");

    for (int i = 0; i < 10; ++i) {

        auto flashLeft = std::make_shared<BillboardMuzzleFlash>();
        flashLeft->Initialize(renderer, muzzleFlashTexture, 2.0f, 0.0f);
        flashLeft->SetFadeOut(false);
        muzzleFlashPoolLeft.push_back(flashLeft);
        renderer->AddGameObject(flashLeft);

        auto flashRight = std::make_shared<BillboardMuzzleFlash>();
        flashRight->Initialize(renderer, muzzleFlashTexture, 2.0f, 0.0f);
        flashRight->SetFadeOut(false);
        muzzleFlashPoolRight.push_back(flashRight);
        renderer->AddGameObject(flashRight);
        
    }


    auto muzzleSmokeTexture = textureManager.LoadTexture(L"Assets/Textures/MuzzleSmoke.png");
    auto noiseTexture = textureManager.LoadTexture(L"Assets/NoiseTextures/noise2.png");

    for (int i = 0; i < 20; ++i) {
        auto smokeLeft = std::make_shared<BillboardMuzzleSmoke>();
        smokeLeft->Initialize(renderer, muzzleSmokeTexture, noiseTexture, 4.0f, 0.0f);
        smokeLeft->SetFadeOut(true);
        muzzleSmokePoolLeft.push_back(smokeLeft);
        renderer->AddGameObject(smokeLeft);

        auto smokeRight = std::make_shared<BillboardMuzzleSmoke>();
        smokeRight->Initialize(renderer, muzzleSmokeTexture, noiseTexture, 4.0f, 0.0f);
        smokeRight->SetFadeOut(true);
        muzzleSmokePoolRight.push_back(smokeRight);
        renderer->AddGameObject(smokeRight);
    }

    // Bullet 초기화
    // BulletMesh 로딩
    std::string bulletPath = "Assets/Bullet/bullet.obj";
    
    bulletTexture = textureManager.LoadTexture(L"Assets/Bullet/Textures/bullet_DefaultMaterial_BaseColor.png");
    bulletMesh = modelLoader.LoadMesh(renderer, bulletPath);

    for (int i = 0; i < 90; ++i) {
        auto bullet = std::make_shared<Bullet>(
            XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, 1), 0.0f, bulletMesh, bulletTexture);
        bullet->Initialize(renderer);
        bullet->SetAlive(false);
        bullet->SetCollisionLayer(CollisionLayer::PlayerProjectile);
        bulletPool.push_back(bullet);
        renderer->AddGameObject(bullet); 
    }
    

    XMFLOAT3 currentPosition = GetPosition();

    PxRigidDynamic* actor = PhysicsManager::GetInstance()->CreateDynamicBox(
        { currentPosition.x, currentPosition.y, currentPosition.z }, {4, 4, 4}, {0, 0, 0}, CollisionLayer::Player);

    PhysicsManager::GetInstance()->SetCollisionResponse(
        CollisionLayer::Player, CollisionLayer::Default, CollisionResponse::Block);

    actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
    actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);

    // 물리 재질 생성 및 반발력, 마찰력 설정
    PxMaterial* material = PhysicsManager::GetInstance()->GetDefaultMaterial();
    material->setRestitution(0.0f);  // 반발력 제거
    material->setStaticFriction(0.0f); // 정적 마찰력 제거
    material->setDynamicFriction(0.0f); // 동적 마찰력 제거

    // actor->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, true);

    // 객체에 물리 재질 설정
    PxShape* shape;
    actor->getShapes(&shape, 1);
    shape->setMaterials(&material, 1);

    BindPhysicsActor(actor);

    if (auto* dynamicActor = physicsActor->is<PxRigidDynamic>()) {
        // Kinematic 설정을 통해 속도나 회전 반응을 차단
        // dyn->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        dynamicActor->setLinearVelocity(PxVec3(0, 0, 0));  // 이동 방지
        dynamicActor->setAngularVelocity(PxVec3(0, 0, 0)); // 회전 방지
    }

    SetColliderOffset(XMFLOAT3(0.0f, 5.0f, 0.0f));



    // Bullet 충돌이벤트 설정

    // 한 번만 등록하도록 static 플래그 사용
    static bool bulletEventHandlersRegistered = false;
    if (!bulletEventHandlersRegistered) {
        bulletEventHandlersRegistered = true;

        // 총알이 적 Flight에 부딪히면 데미지 처리
        CollisionEventHandler hitEnemyHandler;
        hitEnemyHandler.Matches = [](PxActor* a, PxActor* b) {
            auto* objA = static_cast<GameObject*>(a->userData);
            auto* objB = static_cast<GameObject*>(b->userData);
            auto* bullet = dynamic_cast<Bullet*>(objA);
            auto* flight = dynamic_cast<Flight*>(objB);
            return bullet && flight && flight->IsEnemy();
            };

        hitEnemyHandler.Callback = [](PxActor* a, PxActor* b) {
            auto* bullet = static_cast<Bullet*>(a->userData);
            auto* flight = static_cast<Flight*>(b->userData);
            if (bullet && flight) {
                flight->TakeDamage(10.0f);
                bullet->OnHit(flight);
            }
            };

        PhysicsManager::GetInstance()->RegisterHitHandler(hitEnemyHandler);

        // 총알이 비행기가 아닌 다른 물체(지형 등)와 부딪히면 그냥 사라짐
        CollisionEventHandler hitOtherHandler;
        hitOtherHandler.Matches = [](PxActor* a, PxActor* b) {
            auto* objA = static_cast<GameObject*>(a->userData);
            auto* objB = static_cast<GameObject*>(b->userData);
            auto* bullet = dynamic_cast<Bullet*>(objA);
            auto* flight = dynamic_cast<Flight*>(objB);
            return bullet && !flight;  // 비행기가 아닌 대상과 충돌
            };
        hitOtherHandler.Callback = [](PxActor* a, PxActor* b) {
            auto* bullet = static_cast<Bullet*>(a->userData);
            if (bullet) bullet->OnHit(nullptr);
            };
        PhysicsManager::GetInstance()->RegisterHitHandler(hitOtherHandler);
    }

    return true;
}

void PlayerFlight::Update(float deltaTime) {
    InputManager& input = InputManager::GetInstance();
    Flight::Update(deltaTime);

    SyncFromPhysics();  // 물리 상태에서 비행기 상태 동기화

    float yawInput = 0.0f;
    float pitchInput = 0.0f;
    float rollInput = 0.0f;
    float moveInput = 0.0f;

    // 입력 처리
    if (input.IsKeyHeld('Z')) yawInput = -1.0f;
    if (input.IsKeyHeld('C')) yawInput = 1.0f;

    if (input.IsKeyHeld('Q')) pitchInput = -1.0f;
    if (input.IsKeyHeld('E')) pitchInput = 1.0f;

    if (input.IsKeyHeld('A')) rollInput = 1.0f;
    if (input.IsKeyHeld('D')) rollInput = -1.0f;

    if (input.IsKeyHeld('W')) moveInput = 1.0f;
    if (input.IsKeyHeld('S')) moveInput = -1.0f;

    float yawSpeed = XM_PIDIV2;
    float pitchSpeed = XM_PIDIV2;
    float rollSpeed = XM_PIDIV2;

    // 로컬 방향 벡터 계산
    localForward = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), rotationQuat);
    localRight = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), rotationQuat);
    localUp = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), rotationQuat);

    
    // 카메라의 방향을 기준으로 비행기 이동
    // XMFLOAT3 cameraForwardFloat = camera->GetForwardVector();  // 카메라가 향하는 방향 (XMFLOAT3)
    // XMVECTOR cameraForward = XMLoadFloat3(&cameraForwardFloat);  // XMVECTOR로 변환
    // localForward = XMVector3Normalize(cameraForward);  // 카메라 방향으로 맞추기
    

    // 회전 적용 (Yaw, Pitch, Roll)
    XMVECTOR qYaw = XMQuaternionRotationAxis(localUp, yawInput * yawSpeed * deltaTime);
    XMVECTOR qPitch = XMQuaternionRotationAxis(localRight, pitchInput * pitchSpeed * deltaTime);
    XMVECTOR qRoll = XMQuaternionRotationAxis(localForward, rollInput * rollSpeed * deltaTime);

    rotationQuat = XMQuaternionMultiply(rotationQuat, qYaw);
    rotationQuat = XMQuaternionMultiply(rotationQuat, qPitch);
    rotationQuat = XMQuaternionMultiply(rotationQuat, qRoll);
    rotationQuat = XMQuaternionNormalize(rotationQuat);

    // 가속도 기반 속도 조절
    float targetSpeed = 0.0f;
    if (moveInput > 0.0f)       targetSpeed = maxSpeed;
    else if (moveInput < 0.0f)  targetSpeed = -maxSpeed;

    float thrustAccel = 200.0f;
    float dragAccel = 50.0f;

    float accel = 0.0f;
    if (speed < targetSpeed)
        accel = thrustAccel;
    else if (speed > targetSpeed)
        accel = -thrustAccel;

    speed += accel * deltaTime;

    // 감속 적용
    if (targetSpeed == 0.0f) {
        if (speed > 0.0f)
            speed = std::max<float>(0.0f, speed - dragAccel * deltaTime);
        else
            speed = std::min<float>(0.0f, speed + dragAccel * deltaTime);
    }

    speed = std::clamp(speed, -maxSpeed, maxSpeed);

    // 이동 계산 (이동 방향 고려)
    XMVECTOR moveVec = localForward * (speed * deltaTime); 
    XMFLOAT3 pos = GetPosition();
    XMVECTOR posVec = XMLoadFloat3(&pos) + moveVec;

    XMFLOAT3 updatedPos;
    XMStoreFloat3(&updatedPos, posVec);
    SetPosition(updatedPos);
    SetRotationQuat(rotationQuat);
    

    cameraController.Update(camera.get(), updatedPos, deltaTime);

    // 카메라 회전을 기준으로 비행기 회전 보간 적용

    if (rollInput == 0.0f) {

        XMVECTOR cameraQuat = camera->GetRotationQuat();
        float followSpeed = 2.0f; // 보간 속도 계수
        rotationQuat = XMQuaternionSlerp(rotationQuat, cameraQuat, followSpeed * deltaTime);
        rotationQuat = XMQuaternionNormalize(rotationQuat);
        SetRotationQuat(rotationQuat);
        


        /*
        // 각도에 따라 자연스러운 Slerp 비율 계산
        XMVECTOR currentQuat = rotationQuat;
        XMVECTOR targetQuat = camera->GetRotationQuat();

        // 내적 및 반전 처리
        float dot = XMVectorGetX(XMQuaternionDot(currentQuat, targetQuat));
        if (dot < 0.0f)
        {
            targetQuat = XMVectorNegate(targetQuat);
            dot = -dot;
        }
        dot = std::clamp(dot, -1.0f, 1.0f);

        // 각도 계산
        float angle = acosf(dot) * 2.0f;
        if (angle < 1e-5f)
            return;

        // 1. Slerp 보간 비율 계산 (감속 느낌 살림)
        float slerpSpeed = 2.0f; // 감속 보간 계수
        float t = slerpSpeed * deltaTime;
        t = std::min(t, 1.0f); 

        // 2. Slerp 수행
        XMVECTOR newQuat = XMQuaternionSlerp(currentQuat, targetQuat, t);

        // 3. 실제 회전된 각도 측정
        float newDot = XMVectorGetX(XMQuaternionDot(currentQuat, newQuat));
        float newAngle = acosf(std::clamp(newDot, -1.0f, 1.0f)) * 2.0f;

        // 4. 최대 회전량 제한
        float maxAnglePerSecond = XMConvertToRadians(90.0f);
        float maxStep = maxAnglePerSecond * deltaTime;

        if (newAngle > maxStep)
        {
            float limitedT = maxStep / angle;
            newQuat = XMQuaternionSlerp(currentQuat, targetQuat, limitedT);
        }

        rotationQuat = XMQuaternionNormalize(newQuat);
        SetRotationQuat(rotationQuat);
        */
    }

    
    UpdateWorldMatrix();
    ApplyTransformToPhysics();

    DrawPhysicsColliderDebug();

    lastFireTime += deltaTime;

    if (input.IsMouseButtonDown(MouseButton::Left)) {
        Fire();
    }
}

void PlayerFlight::Render(Renderer* renderer)
{
    renderer->SetCamera(camera);

    Flight::Render(renderer);
}

void PlayerFlight::Fire()
{
    if (lastFireTime < fireCooldown)
        return; // 쿨타임이 지나지 않았으면 발사하지 않음

    lastFireTime = 0.0f; // 타이머 리셋

    // DebugManager::GetInstance().AddOnScreenMessage("Fire!", 5.0f);

    XMVECTOR worldUpVec = XMVectorSet(0, 1, 0, 0);

    XMFLOAT3 flightPos = GetPosition();
    XMVECTOR flightPosVec = XMLoadFloat3(&flightPos);
    XMVECTOR flightForwardVec = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), rotationQuat);
    XMVECTOR flightRightVec = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), rotationQuat);
    XMVECTOR flightUpVec = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), rotationQuat);
    XMVECTOR muzzleOffset = XMVectorSet(0.5f, -0.1f, 2.0f, 0);

    XMVECTOR leftMuzzlePosVec = flightPosVec - flightRightVec * 5.0f + flightForwardVec * 10.0f + flightUpVec * 5.0f;
    XMVECTOR rightMuzzlePosVec = flightPosVec + flightRightVec * 5.0f + flightForwardVec * 10.0f + flightUpVec * 5.0f;

    XMFLOAT3 leftMuzzlePos, rightMuzzlePos;
    XMStoreFloat3(&leftMuzzlePos, leftMuzzlePosVec);
    XMStoreFloat3(&rightMuzzlePos, rightMuzzlePosVec);

    for (auto& flash : muzzleFlashPoolLeft) {
        if (!flash->IsAlive()) {
            flash->SetPosition(leftMuzzlePos);
            flash->SetLifetime(0.008f);
            break;
        }
    }

    for (auto& flash : muzzleFlashPoolRight) {
        if (!flash->IsAlive()) {
            flash->SetPosition(rightMuzzlePos);
            flash->SetLifetime(0.008f);
            break;
        }
    }

    const float smokeSpreadRadius = 1.0f;

    for (auto& smoke : muzzleSmokePoolLeft) {
        if (!smoke->IsAlive()) {
            XMFLOAT3 offset = RandomOffset3D(smokeSpreadRadius);
            XMFLOAT3 finalPos = {
                leftMuzzlePos.x + offset.x,
                leftMuzzlePos.y + offset.y,
                leftMuzzlePos.z + offset.z
            };
            smoke->SetPosition(finalPos);
            smoke->SetLifetime(2.0f);
            break;
        }
    }

    for (auto& smoke : muzzleSmokePoolRight) {
        if (!smoke->IsAlive()) {
            XMFLOAT3 offset = RandomOffset3D(smokeSpreadRadius);
            XMFLOAT3 finalPos = {
                rightMuzzlePos.x + offset.x,
                rightMuzzlePos.y + offset.y,
                rightMuzzlePos.z + offset.z
            };
            smoke->SetPosition(finalPos);
            smoke->SetLifetime(2.0f);
            break;
        }
    }

    XMFLOAT3 bulletDirection;
    XMStoreFloat3(&bulletDirection, XMVector3Normalize(flightForwardVec));

    float bulletSpeed = 600.0f;

    // 람다함수
    auto fireBullet = [&](const XMFLOAT3& spawnPos)
        {
            for (auto& bullet : bulletPool) {
                if (!bullet->IsAlive()) {

                    // Activate
                    bullet->Activate(spawnPos, bulletDirection, bulletSpeed);
                    bullet->SetMaxLifeTime(3.0f);

                    // 총알의 local forward 기준 (예: -X 기준으로 모델링된 경우)
                    XMVECTOR bulletLocalForward = XMVectorSet(-1, 0, 0, 0);
                    XMVECTOR bulletForwardVec = XMVector3Rotate(bulletLocalForward, rotationQuat);
                    XMVECTOR bulletUpVec = flightUpVec;
                    XMVECTOR bulletRightVec = XMVector3Normalize(XMVector3Cross(bulletUpVec, bulletForwardVec));
                   

                    // 회전 행렬 생성
                    XMMATRIX bulletRotMatrix = XMMatrixIdentity();
                    bulletRotMatrix.r[0] = bulletRightVec;
                    bulletRotMatrix.r[1] = bulletUpVec;
                    bulletRotMatrix.r[2] = bulletForwardVec;

                    // 쿼터니언 변환
                    XMVECTOR bulletRotationQuat = XMQuaternionRotationMatrix(bulletRotMatrix);

                    // PhysX 회전 적용
                    if (auto* actor = static_cast<physx::PxRigidDynamic*>(bullet->GetPhysicsActor())) {
                        XMFLOAT4 quatFloat4;
                        XMStoreFloat4(&quatFloat4, bulletRotationQuat);
                        PxQuat pxQuat(quatFloat4.x, quatFloat4.y, quatFloat4.z, quatFloat4.w);
                        actor->setGlobalPose(PxTransform(PxVec3(spawnPos.x, spawnPos.y, spawnPos.z), pxQuat));
                    }

                    break;
                }
            }
        };

    fireBullet(leftMuzzlePos);
    fireBullet(rightMuzzlePos);
}
