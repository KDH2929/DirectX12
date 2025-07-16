#pragma once

#include <DirectXMath.h>
using namespace DirectX;

class Camera {
public:
    Camera();

    void SetPosition(const XMFLOAT3& position);
    void SetRotationEuler(const XMFLOAT3& euler);      // ���Ϸ� -> ���ʹϾ� ��ȯ
    void SetRotationQuat(const XMVECTOR& quat);        // ���ʹϾ� ����

    XMFLOAT3 GetPosition() const;
    XMFLOAT3 GetRotationEuler() const;
    XMVECTOR GetRotationQuat() const;

    void SetOrthographic(float width, float height, float nearZ, float farZ);
    void SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ);

    void UpdateViewMatrix();
    XMMATRIX GetViewMatrix() const;
    XMMATRIX GetProjectionMatrix() const;

    XMFLOAT3 GetForwardVector() const;
    XMFLOAT3 GetUpVector() const;

    void LookAt(XMVECTOR eye, XMVECTOR target);

private:
    XMFLOAT3 position;
    XMFLOAT3 rotationEuler;
    XMVECTOR rotationQuat;

    XMMATRIX viewMatrix;
    XMMATRIX projectionMatrix;
};
