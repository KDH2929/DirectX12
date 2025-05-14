#include "Camera.h"

Camera::Camera()
    : position(0.0f, 0.0f, -5.0f),
    rotationEuler(0.0f, 0.0f, 0.0f),
    rotationQuat(XMQuaternionIdentity()),
    viewMatrix(XMMatrixIdentity()),
    projectionMatrix(XMMatrixIdentity())
{
    UpdateViewMatrix();
}

void Camera::SetPosition(const XMFLOAT3& pos) {
    position = pos;
}

void Camera::SetRotationEuler(const XMFLOAT3& euler) {
    rotationEuler = euler;
    rotationQuat = XMQuaternionRotationRollPitchYaw(euler.x, euler.y, euler.z);
}

void Camera::SetRotationQuat(const XMVECTOR& quat) {
    rotationQuat = XMQuaternionNormalize(quat);

    XMFLOAT4 q;
    XMStoreFloat4(&q, rotationQuat);
    rotationEuler = {
        asinf(2.0f * (q.w * q.x - q.y * q.z)),
        atan2f(2.0f * (q.w * q.y + q.x * q.z), 1.0f - 2.0f * (q.y * q.y + q.x * q.x)),
        atan2f(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.z * q.z + q.x * q.x))
    };
}

XMFLOAT3 Camera::GetPosition() const {
    return position;
}

XMFLOAT3 Camera::GetRotationEuler() const {
    return rotationEuler;
}

XMVECTOR Camera::GetRotationQuat() const {
    return rotationQuat;
}

void Camera::SetOrthographic(float width, float height, float nearZ, float farZ) {
    projectionMatrix = XMMatrixOrthographicLH(width, height, nearZ, farZ);
}

void Camera::SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ) {
    projectionMatrix = XMMatrixPerspectiveFovLH(fovY, aspectRatio, nearZ, farZ);
}

void Camera::UpdateViewMatrix() {
    XMVECTOR eye = XMLoadFloat3(&position);

    // È¸Àü ÄõÅÍ´Ï¾ð ¡æ ¹æÇâ º¤ÅÍ
    XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), rotationQuat);
    XMVECTOR up = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), rotationQuat);
    XMVECTOR target = eye + forward;

    viewMatrix = XMMatrixLookAtLH(eye, target, up);
}

XMMATRIX Camera::GetViewMatrix() const {
    return viewMatrix;
}

XMMATRIX Camera::GetProjectionMatrix() const {
    return projectionMatrix;
}

XMFLOAT3 Camera::GetForwardVector() const
{
    XMVECTOR defaultForward = XMVectorSet(0, 0, 1, 0);
    XMVECTOR rotated = XMVector3Rotate(defaultForward, rotationQuat);

    XMFLOAT3 forward;
    XMStoreFloat3(&forward, rotated);
    return forward;
}

void Camera::LookAt(XMVECTOR eye, XMVECTOR target) {
    viewMatrix = XMMatrixLookAtLH(eye, target, XMVectorSet(0, 1, 0, 0));
}
