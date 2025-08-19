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

void DirectionalLight::Update(Camera* camera)
{
    const float orthoWidth = 200.0f;
    const float orthoHeight = 200.0f;
    const float nearZ = 1.0f;
    const float farZ = 200.0f;
    const float shadowDistance = farZ * 0.5f;
    constexpr float epsilon = 1e-6f;

    XMVECTOR lightDir = XMLoadFloat3(&lightData.direction);

    // 길이 작으면 아래 방향으로 대체, 아니면 정규화
    float lengthSq = XMVectorGetX(XMVector3Dot(lightDir, lightDir));
    if (lengthSq < epsilon)
    {
        // 기본 태양빛은 위에서 아래로
        lightDir = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
    }
    else
    {
        lightDir = XMVector3Normalize(lightDir);
    }

    XMFLOAT3 camPosFloat = camera->GetPosition();
    XMVECTOR camPos = XMLoadFloat3(&camPosFloat);
    XMVECTOR lightPos = camPos - lightDir * shadowDistance;
    XMVECTOR target = lightPos + lightDir;

    // up 벡터: 기본 Y축, 너무 평행하면 X축으로
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (fabsf(XMVectorGetX(XMVector3Dot(up, lightDir))) > 0.99f)
    {
        up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    }

    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, target, up);
    XMMATRIX lightProj = XMMatrixOrthographicLH(orthoWidth, orthoHeight, nearZ, farZ);

    shadowViewProjMatrices[0] = lightView * lightProj;
}