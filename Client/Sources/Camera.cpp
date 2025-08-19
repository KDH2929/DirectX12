#include "Camera.h"
#include <DirectXMath.h>
using namespace DirectX;

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

    XMMATRIX R = XMMatrixRotationQuaternion(rotationQuat);

    // 2) 매트릭스 원소 꺼내기
    //    R.r[2] 는 forward 벡터 (m20, m21, m22)
    float m20 = XMVectorGetX(R.r[2]);
    float m21 = XMVectorGetY(R.r[2]);
    float m22 = XMVectorGetZ(R.r[2]);
    float m10 = XMVectorGetX(R.r[1]); // up.y는 R.r[1].m128_f32[1]

    // 3) Euler 각 계산 (Left-Handed, Y-up 기준)
    float pitch = asinf(-m21);
    float yaw = atan2f(m20, m22);
    float roll = atan2f(m10, XMVectorGetY(R.r[1]));

    rotationEuler = { pitch, yaw, roll };
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

    // 회전 쿼터니언 → 방향 벡터
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

XMFLOAT3 Camera::GetUpVector() const
{
    XMVECTOR defaultUp = XMVectorSet(0, 1, 0, 0);
    XMVECTOR rotated = XMVector3Rotate(defaultUp, rotationQuat);

    XMFLOAT3 up;
    XMStoreFloat3(&up, rotated);
    return up;
}

void Camera::LookAt(XMVECTOR eye, XMVECTOR target) {
    viewMatrix = XMMatrixLookAtLH(eye, target, XMVectorSet(0, 1, 0, 0));
}
