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

void SpotLight::Update(Camera* camera)
{
    XMVECTOR lightPos = XMLoadFloat3(&lightData.position);
    XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&lightData.direction));
    XMVECTOR target = XMVectorAdd(lightPos, lightDir);

    // upVector
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    float    cosAngle = XMVectorGetX(XMVector3Dot(up, lightDir));
    if (fabsf(cosAngle) > 0.99f)
        up = XMVectorSet(1, 0, 0, 0);

    XMMATRIX view = XMMatrixLookAtLH(lightPos, target, up);

    // FOV = outer cutoff angle * 2
    float fov = lightData.outerCutoffAngle * 2.0f;
    float aspect = 1.0f;
    float nearZ = 0.1f;

    // 감쇠 계수로부터 섀도우 farZ 계산 (예: 1% threshold)
    float C = lightData.constant;
    float L = lightData.linear;
    float Q = lightData.quadratic;
    float farZ = ComputeShadowFarZ(C, L, Q, 0.01f);

    XMMATRIX proj = XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ);

    shadowViewProjMatrices[0] = view * proj;
}

float SpotLight::ComputeShadowFarZ(float constant, float linear, float quadratic, float threshold)
{
    // 1/(C + L·d + Q·d²) = threshold  ⇒  C + L·d + Q·d² = 1/threshold
    float target = 1.0f / threshold - constant;
    // Q·d² + L·d - target = 0
    float discr = linear * linear + 4.0f * quadratic * target;
    if (discr < 0) discr = 0;
    // 양의 해만 취함
    return (-linear + sqrtf(discr)) / (2.0f * quadratic);
}
