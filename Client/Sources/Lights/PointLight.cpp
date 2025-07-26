#include "PointLight.h"
#include <DirectXMath.h>

using namespace DirectX;

PointLight::PointLight()
{
    lightData.type = static_cast<int>(LightType::Point);
    shadowViewProjMatrices.resize(6); // Å¥ºê¸Ê 6¸é
}

LightType PointLight::GetType() const
{
    return LightType::Point;
}

void PointLight::Update(const XMFLOAT3& cameraPosition)
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
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, 0.1f, lightData.falloffEnd); // 90µµ, 1:1, depth 1~100

    for (int i = 0; i < 6; ++i) {
        XMMATRIX view = XMMatrixLookAtLH(lightPos, lightPos + directions[i], ups[i]);
        shadowViewProjMatrices[i] = XMMatrixMultiply(view, proj);
    }
}
