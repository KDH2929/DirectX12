#pragma once

#include "Flight.h"
#include "InputManager.h"
#include "Camera.h"
#include "CameraController.h"
#include "BillboardMuzzleFlash.h"
#include "BillboardMuzzleSmoke.h"
#include "Bullet.h"

class PlayerFlight : public Flight {
public:
    PlayerFlight(std::shared_ptr<Mesh> mesh, std::shared_ptr<Texture> texture);
    ~PlayerFlight() override = default;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;


private:
    void Fire();


private:
    // 속도 및 추력 관련
    float speed = 0.0f;              // 현재 속도
    float maxSpeed = 300.0f;         // 최대 속도
    float cruiseSpeed = 150.0f;      // 순항 속도

    float maxThrustAccel = 200.0f;   // 최대 추력 가속도
    float dragCoefficient = 0.01f;   // 항력 계수 (속도 제곱 기반)

    float lastFireTime = 0.0f;       // 마지막 발사 시간
    float fireCooldown = 0.1f;       // 최소 발사 간격 (초 단위)

    XMVECTOR localForward = XMVectorSet(0, 0, 1, 0);
    XMVECTOR localRight = XMVectorSet(1, 0, 0, 0);
    XMVECTOR localUp = XMVectorSet(0, 1, 0, 0);


    // 카메라
    std::shared_ptr<Camera> camera;
    CameraController cameraController;
    XMVECTOR cameraQuat = XMQuaternionIdentity();

    std::vector<std::shared_ptr<BillboardMuzzleFlash>> muzzleFlashPoolLeft;
    std::vector<std::shared_ptr<BillboardMuzzleFlash>> muzzleFlashPoolRight;

    std::vector<std::shared_ptr<BillboardMuzzleSmoke>> muzzleSmokePoolLeft;
    std::vector<std::shared_ptr<BillboardMuzzleSmoke>> muzzleSmokePoolRight;


    std::shared_ptr<Mesh> bulletMesh;
    std::shared_ptr<Texture> bulletTexture;
    std::vector<std::shared_ptr<Bullet>> bulletPool;

};
