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

    // Ÿ��(���� ���ϴ� ��ġ)
    XMVECTOR target = lightPos + lightDir;

    // up ���� �⺻�� (Y��)
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // lightDir �� up �� �ʹ� �����ϴٸ� Z������ ��ü
    float cosAngle = XMVectorGetX(XMVector3Dot(up, lightDir));
    if (fabsf(cosAngle) > 0.99f)
    {
        up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    }

    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, target, up);
    XMMATRIX lightProj = XMMatrixOrthographicLH(orthoWidth, orthoHeight, nearZ, farZ);

    shadowViewProjMatrices[0] = XMMatrixMultiply(lightView, lightProj);
}