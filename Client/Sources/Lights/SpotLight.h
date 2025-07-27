#pragma once

#include "BaseLight.h"

class SpotLight : public BaseLight
{
public:
    SpotLight();

    LightType GetType() const override;
    void Update(Camera* camera) override;

private:
    float ComputeShadowFarZ(float constant, float linear, float quadratic, float threshold = 0.01f);
};
