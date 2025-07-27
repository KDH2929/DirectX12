#include "PointLight.h"
#include <DirectXMath.h>

using namespace DirectX;

PointLight::PointLight()
{
    lightData.type = static_cast<int>(LightType::Point);
    shadowViewProjMatrices.resize(6); // 큐브맵 6면
}

LightType PointLight::GetType() const
{
    return LightType::Point;
}

void PointLight::Update(Camera* camera)
{
    static const XMVECTOR directions[6] = {
        XMVectorSet(1,  0,  0, 0), XMVectorSet(-1,  0,  0, 0),
        XMVectorSet(0,  1,  0, 0), XMVectorSet(0, -1,  0, 0),
        XMVectorSet(0,  0,  1, 0), XMVectorSet(0,  0, -1, 0),
    };

    static const XMVECTOR ups[6] = {
        XMVectorSet(0, 1, 0, 0), XMVectorSet(0, 1, 0, 0),
        XMVectorSet(0, 0, -1, 0), XMVectorSet(0, 0, 1, 0),
        XMVectorSet(0, 1, 0, 0), XMVectorSet(0, 1, 0, 0),
    };

    XMVECTOR lightPos = XMLoadFloat3(&lightData.position);

    // 감쇠가 1% 이하로 떨어지는 거리를 farZ로
    float nearZ = 0.1f;
    float farZ = ComputeShadowFarZ(lightData.constant, lightData.linear, lightData.quadratic, 0.01f); 

    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        XM_PIDIV2,    // 90도
        1.0f,         // 정사각
        nearZ,
        farZ
    );

    for (int i = 0; i < 6; ++i) {
        XMMATRIX view = XMMatrixLookAtLH(lightPos, lightPos + directions[i], ups[i]);
        shadowViewProjMatrices[i] = XMMatrixMultiply(view, proj);
    }
}

float PointLight::ComputeShadowFarZ(float constant, float linear, float quadratic, float threshold)
{
    // 1/(C + L·d + Q·d²) = threshold  ⇒  C + L·d + Q·d² = 1/threshold
    float target = 1.0f / threshold - constant;
    // Q·d² + L·d - target = 0
    float discr = linear * linear + 4.0f * quadratic * target;
    if (discr < 0) discr = 0;
    // 양의 해만 취함
    return (-linear + sqrtf(discr)) / (2.0f * quadratic);
}
