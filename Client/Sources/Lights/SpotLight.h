#pragma once

#include "BaseLight.h"

class SpotLight : public BaseLight
{
public:
    SpotLight();

    LightType GetType() const override;
    void Update(const XMFLOAT3& cameraPosition) override;
};
