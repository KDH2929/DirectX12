#pragma once

#include <vector>
#include <memory>
#include <wrl/client.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include "Lights/BaseLight.h"
#include "Lights/LightData.h"
#include "ConstantBuffers.h"
#include "ShadowMap.h"
#include "Camera.h"

using namespace DirectX;

class Renderer;

class LightingManager
{
public:
    LightingManager() = default;

    void AddLight(std::shared_ptr<BaseLight> light);
    void ClearLights();

    void Update(Renderer* renderer);
    void UpdateImGui();
    const std::vector<std::shared_ptr<BaseLight>>& GetLights() const;

private:
    void UploadLightingBuffer(Renderer* renderer);
    void UploadShadowViewProjBuffer(Renderer* renderer);

    std::vector<std::shared_ptr<BaseLight>> lights;
    CB_Lighting lightingData = {};
};