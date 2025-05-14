#include "CameraController.h"
#include <algorithm>

using namespace DirectX;

CameraController::CameraController(float distance, float height)
    : cameraDistance(distance), cameraHeight(height), rotationSpeed(0.002f), mode(CameraMode::ThirdPerson) {}

void CameraController::SetCameraMode(CameraMode newMode) {
    mode = newMode;
    yaw = 0.0f;
    pitch = 0.0f;
}

void CameraController::SetCameraOffset(float distance, float height) {
    cameraDistance = distance;
    cameraHeight = height;
}

void CameraController::Update(Camera* camera, const XMFLOAT3& modelPosition, float deltaTime) {
    switch (mode) {
    case CameraMode::FirstPerson:
        UpdateFirstPerson(camera, modelPosition, deltaTime);
        break;
    case CameraMode::ThirdPerson:
        UpdateThirdPerson(camera, modelPosition, deltaTime);
        break;
    case CameraMode::Freeform:
        UpdateFreeform(camera, modelPosition, deltaTime);
        break;
    case CameraMode::FlightThirdPerson:
        UpdateFlightThirdPerson(camera, modelPosition, deltaTime);
        break;
    }
}

void CameraController::UpdateFirstPerson(Camera* camera, const XMFLOAT3& modelPosition, float deltaTime) {
    InputManager& input = InputManager::GetInstance();

    yaw -= input.GetMouseDeltaX() * rotationSpeed;
    pitch -= input.GetMouseDeltaY() * rotationSpeed;

    pitch = std::clamp(pitch, -XM_PIDIV2 + 0.1f, XM_PIDIV2 - 0.1f);

    XMVECTOR qPitch = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), pitch);
    XMVECTOR qYaw = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), yaw);
    XMVECTOR cameraRot = XMQuaternionMultiply(qYaw, qPitch);
    cameraRot = XMQuaternionNormalize(cameraRot);

    camera->SetPosition(modelPosition);
    camera->SetRotationQuat(cameraRot);
    camera->UpdateViewMatrix();
}

void CameraController::UpdateThirdPerson(Camera* camera, const XMFLOAT3& modelPosition, float deltaTime) {
    InputManager& input = InputManager::GetInstance();

    yaw -= input.GetMouseDeltaX() * rotationSpeed;
    pitch -= input.GetMouseDeltaY() * rotationSpeed;
    pitch = std::clamp(pitch, -XM_PIDIV2 + 0.1f, XM_PIDIV2 - 0.1f);

    XMVECTOR qPitch = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), pitch);
    XMVECTOR qYaw = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), yaw);
    XMVECTOR cameraRot = XMQuaternionMultiply(qYaw, qPitch);
    cameraRot = XMQuaternionNormalize(cameraRot);

    XMVECTOR offset = XMVectorSet(0.0f, cameraHeight, -cameraDistance, 0.0f);
    XMVECTOR rotatedOffset = XMVector3Rotate(offset, cameraRot);

    XMVECTOR modelPos = XMLoadFloat3(&modelPosition);
    XMVECTOR cameraPos = modelPos + rotatedOffset;

    XMFLOAT3 finalCamPos;
    XMStoreFloat3(&finalCamPos, cameraPos);
    camera->SetPosition(finalCamPos);
    camera->SetRotationQuat(cameraRot);
    camera->UpdateViewMatrix();
}

void CameraController::UpdateFlightThirdPerson(Camera* camera, const XMFLOAT3& modelPosition, float deltaTime) {
    InputManager& input = InputManager::GetInstance();

    float dx = static_cast<float>(input.GetMouseDeltaX());
    float dy = static_cast<float>(input.GetMouseDeltaY());

    float yawSpeed = 0.002f;
    float pitchSpeed = 0.002f;

    static float yaw = 0.0f;
    static float pitch = 0.0f;

    yaw += dx * yawSpeed;
    pitch += dy * pitchSpeed;

    float maxPitch = XM_PIDIV2 - 0.01f;
    pitch = std::clamp(pitch, -maxPitch, maxPitch);

    XMVECTOR localUp = XMVectorSet(0, 1, 0, 0);
    XMVECTOR localRight = XMVectorSet(1, 0, 0, 0);

    XMVECTOR qYaw = XMQuaternionRotationAxis(localUp, yaw);
    XMVECTOR qPitch = XMQuaternionRotationAxis(localRight, pitch);
    XMVECTOR cameraRot = XMQuaternionMultiply(qPitch, qYaw);
    cameraRot = XMQuaternionNormalize(cameraRot);

    XMVECTOR modelPos = XMLoadFloat3(&modelPosition);
    XMVECTOR offset = XMVectorSet(0.0f, cameraHeight, -cameraDistance, 0.0f);
    XMVECTOR rotatedOffset = XMVector3Rotate(offset, cameraRot);
    XMVECTOR cameraPos = modelPos + rotatedOffset;

    XMFLOAT3 finalPos;
    XMStoreFloat3(&finalPos, cameraPos);
    camera->SetPosition(finalPos);
    camera->SetRotationQuat(cameraRot);
    camera->UpdateViewMatrix();
}

void CameraController::UpdateFreeform(Camera* camera, const XMFLOAT3& modelPosition, float deltaTime) {
    // 자유 시점은 구현 필요 시 여기에 추가
}
