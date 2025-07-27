#include "PointLight.h"
#include <DirectXMath.h>

using namespace DirectX;

PointLight::PointLight()
{
    lightData.type = static_cast<int>(LightType::Point);
    shadowViewProjMatrices.resize(6); // ť��� 6��
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

    // ���谡 1% ���Ϸ� �������� �Ÿ��� farZ��
    float nearZ = 0.1f;
    float farZ = ComputeShadowFarZ(lightData.constant, lightData.linear, lightData.quadratic, 0.01f); 

    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        XM_PIDIV2,    // 90��
        1.0f,         // ���簢
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
    // 1/(C + L��d + Q��d��) = threshold  ��  C + L��d + Q��d�� = 1/threshold
    float target = 1.0f / threshold - constant;
    // Q��d�� + L��d - target = 0
    float discr = linear * linear + 4.0f * quadratic * target;
    if (discr < 0) discr = 0;
    // ���� �ظ� ����
    return (-linear + sqrtf(discr)) / (2.0f * quadratic);
}
