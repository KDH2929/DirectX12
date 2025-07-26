#include "DirectionalLight.h"
#include <DirectXMath.h>

using namespace DirectX;

DirectionalLight::DirectionalLight()
{
    lightData.type = static_cast<int>(LightType::Directional);
    shadowViewProjMatrices.resize(1);
}

LightType DirectionalLight::GetType() const
{
    return LightType::Directional;
}

void DirectionalLight::Update(const XMFLOAT3& cameraPosition)
{
    const float orthoWidth = 500.0f;
    const float orthoHeight = 500.0f;
    const float nearZ = 1.0f;
    const float farZ = 100.0f;
    const float shadowDistance = farZ * 0.5f;

    XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&lightData.direction));
    XMVECTOR camPos = XMLoadFloat3(&cameraPosition);
    XMVECTOR lightPos = camPos - lightDir * shadowDistance;

    // 타겟(빛이 향하는 위치)
    XMVECTOR target = lightPos + lightDir;

    // up 벡터 기본값 (Y축)
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // lightDir 와 up 이 너무 평행하다면 Z축으로 대체
    float cosAngle = XMVectorGetX(XMVector3Dot(up, lightDir));
    if (fabsf(cosAngle) > 0.99f)
    {
        up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    }

    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, target, up);
    XMMATRIX lightProj = XMMatrixOrthographicLH(orthoWidth, orthoHeight, nearZ, farZ);

    shadowViewProjMatrices[0] = XMMatrixMultiply(lightView, lightProj);
}