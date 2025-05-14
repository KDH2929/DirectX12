#pragma once

#include <DirectXMath.h>
using namespace DirectX;

class Camera {
public:
    Camera();

    void SetPosition(const XMFLOAT3& position);
    void SetRotationEuler(const XMFLOAT3& euler);      // 오일러 -> 쿼터니언 변환
    void SetRotationQuat(const XMVECTOR& quat);        // 쿼터니언 설정

    XMFLOAT3 GetPosition() const;
    XMFLOAT3 GetRotationEuler() const;
    XMVECTOR GetRotationQuat() const;

    void SetOrthographic(float width, float height, float nearZ, float farZ);
    void SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ);

    void UpdateViewMatrix();
    XMMATRIX GetViewMatrix() const;
    XMMATRIX GetProjectionMatrix() const;

    XMFLOAT3 GetForwardVector() const;

    void LookAt(XMVECTOR eye, XMVECTOR target);

private:
    XMFLOAT3 position;
    XMFLOAT3 rotationEuler;
    XMVECTOR rotationQuat;

    XMMATRIX viewMatrix;
    XMMATRIX projectionMatrix;
};
