#include "SpotLight.h"
#include <DirectXMath.h>

using namespace DirectX;

SpotLight::SpotLight()
{
    lightData.type = static_cast<int>(LightType::Spot);
    shadowViewProjMatrices.resize(1);
}

LightType SpotLight::GetType() const
{
    return LightType::Spot;
}

void SpotLight::Update(const XMFLOAT3& cameraPosition)
{
    XMVECTOR lightPos = XMLoadFloat3(&lightData.position);
    XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&lightData.direction));
    XMVECTOR target = XMVectorAdd(lightPos, lightDir);

    XMVECTOR up = XMVectorSet(0, 1, 0, 0);

    float cosAngle = XMVectorGetX(XMVector3Dot(up, lightDir));
    if (fabsf(cosAngle) > 0.99f)
    {
        up = XMVectorSet(1, 0, 0, 0);
    }

    XMMATRIX view = XMMatrixLookAtLH(lightPos, target, up);

    float fov = lightData.specPower;
    float aspectRatio = 1.0f;
    float nearZ = 1.0f;
    float farZ = lightData.falloffEnd;
    XMMATRIX proj = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearZ, farZ);

    shadowViewProjMatrices[0] = view * proj;
}