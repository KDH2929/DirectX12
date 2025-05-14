#pragma once

#include "Camera.h"
#include "InputManager.h"
#include <DirectXMath.h>

using namespace DirectX;

enum class CameraMode {
    FirstPerson,
    ThirdPerson,
    Freeform,
    FlightThirdPerson
};

class CameraController {
public:
    CameraController(float distance = 20.0f, float height = 5.0f);

    void SetCameraMode(CameraMode newMode);
    void SetCameraOffset(float distance, float height);
    void Update(Camera* camera, const XMFLOAT3& modelPosition, float deltaTime);

private:
    float cameraDistance;
    float cameraHeight;
    float rotationSpeed;
    CameraMode mode;

    float yaw = 0.0f;
    float pitch = 0.0f;

    void UpdateFirstPerson(Camera* camera, const XMFLOAT3& modelPosition, float deltaTime);
    void UpdateThirdPerson(Camera* camera, const XMFLOAT3& modelPosition, float deltaTime);
    void UpdateFreeform(Camera* camera, const XMFLOAT3& modelPosition, float deltaTime);
    void UpdateFlightThirdPerson(Camera* camera, const XMFLOAT3& modelPosition, float deltaTime);
};
