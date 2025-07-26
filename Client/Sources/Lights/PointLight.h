#pragma once

#include "BaseLight.h"

class PointLight : public BaseLight
{
public:
    PointLight();

    LightType GetType() const override;
    void Update(const XMFLOAT3& cameraPosition) override;
};