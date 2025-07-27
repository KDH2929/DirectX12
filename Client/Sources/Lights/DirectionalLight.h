#pragma once

#include "BaseLight.h"

class DirectionalLight : public BaseLight
{
public:
    DirectionalLight();

    LightType GetType() const override;
    void Update(Camera* camera) override;
};