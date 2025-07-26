#pragma once

#include <DirectXMath.h>
#include <vector>
#include "LightData.h"
#include "LightType.h"

using namespace DirectX;

class BaseLight
{
public:
    virtual ~BaseLight() = default;

    virtual LightType GetType() const = 0;

    virtual void Update(const XMFLOAT3& cameraPosition) = 0;

    virtual bool IsShadowCastingEnabled() const { return shadowCastingEnabled; }

    virtual size_t GetShadowViewProjMatrixCount() const { return shadowViewProjMatrices.size(); }
    virtual const XMMATRIX& GetShadowViewProjMatrix(size_t index = 0) const { return shadowViewProjMatrices[index]; }
    const std::vector<XMMATRIX>& GetShadowViewProjMatrices() const { return shadowViewProjMatrices; }

    void SetStrength(const XMFLOAT3& strength) { lightData.strength = strength; }
    void SetColor(const XMFLOAT3& color) { lightData.color = color; }
    void SetPosition(const XMFLOAT3& position) { lightData.position = position; }
    void SetDirection(const XMFLOAT3& direction) { lightData.direction = direction; }
    void SetFalloff(float start, float end) { lightData.falloffStart = start; lightData.falloffEnd = end; }
    void SetSpecularPower(float power) { lightData.specPower = power; }

    const XMFLOAT3& GetPosition() const { return lightData.position; }
    const XMFLOAT3& GetDirection() const { return lightData.direction; }

    const LightData& GetLightData() const { return lightData; }
    void SetLightData(const LightData& data) { lightData = data; }

    void SetShadowCastingEnabled(bool enabled) { shadowCastingEnabled = enabled; }

protected:
    LightData lightData = {};
    bool shadowCastingEnabled = true;
    std::vector<XMMATRIX> shadowViewProjMatrices;
};
