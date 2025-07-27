#pragma once

#include <DirectXMath.h>
#include <vector>
#include "LightData.h"
#include "LightType.h"
#include "Camera.h"

using namespace DirectX;

class BaseLight
{
public:
    BaseLight() { lightData.shadowCastingEnabled = 1; }
    virtual ~BaseLight() = default;

    virtual LightType GetType() const = 0;

    virtual void Update(Camera* camera) = 0;

    virtual bool IsShadowCastingEnabled() const { return lightData.shadowCastingEnabled != 0; }

    virtual size_t GetShadowViewProjMatrixCount() const { return shadowViewProjMatrices.size(); }
    virtual const XMMATRIX& GetShadowViewProjMatrix(size_t index = 0) const { return shadowViewProjMatrices[index]; }
    const std::vector<XMMATRIX>& GetShadowViewProjMatrices() const { return shadowViewProjMatrices; }

    void SetStrength(const float strength) { lightData.strength = strength; }
    void SetColor(const XMFLOAT3& color) { lightData.color = color; }
    void SetPosition(const XMFLOAT3& position) { lightData.position = position; }
    void SetDirection(const XMFLOAT3& direction) { lightData.direction = direction; }
    void SetCutoffAngles(float innerAngleRad, float outerAngleRad) { lightData.innerCutoffAngle = innerAngleRad; lightData.outerCutoffAngle = outerAngleRad; }
    void SetAttenuation(float constant, float linear, float quadratic) { lightData.constant = constant; lightData.linear = linear; lightData.quadratic = quadratic; }

    const XMFLOAT3& GetPosition() const { return lightData.position; }
    const XMFLOAT3& GetDirection() const { return lightData.direction; }

    const LightData& GetLightData() const { return lightData; }
    void SetLightData(const LightData& data) { lightData = data; }

    void SetShadowCastingEnabled(bool enabled) { lightData.shadowCastingEnabled = enabled ? 1 : 0; }

protected:
    LightData               lightData = {};
    std::vector<XMMATRIX>   shadowViewProjMatrices;
};
