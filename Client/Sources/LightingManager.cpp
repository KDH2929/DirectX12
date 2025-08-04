#include "LightingManager.h"
#include "Renderer.h"
#include "FrameResource/FrameResource.h"
#include <imgui.h>
#include <algorithm>

using namespace DirectX;

void LightingManager::AddLight(std::shared_ptr<BaseLight> light)
{
    if (lights.size() < MAX_LIGHTS)
    {
        lights.push_back(std::move(light));
    }
}

void LightingManager::ClearLights()
{
    lights.clear();
}

void LightingManager::Update(Renderer* renderer)
{
    Camera* camera = renderer->GetCamera();

    lightingData = {};
    lightingData.cameraWorld = camera->GetPosition();

    size_t count = std::min<size_t>(lights.size(), static_cast<size_t>(MAX_LIGHTS));
    for (size_t i = 0; i < count; ++i)
    {
        auto& light = lights[i];
        lightingData.lights[i] = light->GetLightData();

        if (light->IsShadowCastingEnabled())
        {
            light->Update(camera);
        }
    }

    UpdateImGui();
    UploadLightingBuffer(renderer);
    UploadShadowViewProjBuffer(renderer);
}

void LightingManager::UpdateImGui()
{
    ImGui::Begin("Lighting Controls");
    for (size_t i = 0; i < lights.size(); ++i)
    {
        auto& light = lights[i];
        LightData data = light->GetLightData();
        LightType type = light->GetType();

        ImGui::Separator();
        ImGui::Text("%s Light",
            type == LightType::Directional ? "Directional" :
            type == LightType::Point ? "Point" :
            "Spot");

        ImGui::ColorEdit3(("Color##" + std::to_string(i)).c_str(), reinterpret_cast<float*>(&data.color));
        ImGui::SliderFloat(("Strength##" + std::to_string(i)).c_str(), &data.strength, 0.0f, 10.0f);

        if (type != LightType::Directional)
        {
            ImGui::DragFloat3(("Position##" + std::to_string(i)).c_str(), reinterpret_cast<float*>(&data.position), 0.1f, -50.0f, 50.0f);
        }

        if (type != LightType::Point)
        {
            ImGui::DragFloat3(("Direction##" + std::to_string(i)).c_str(), reinterpret_cast<float*>(&data.direction), 0.01f, -1.0f, 1.0f);
        }

        if (type == LightType::Spot)
        {
            float inner = XMConvertToDegrees(data.innerCutoffAngle);
            float outer = XMConvertToDegrees(data.outerCutoffAngle);
            if (ImGui::SliderFloat(("InnerCutoff##" + std::to_string(i)).c_str(), &inner, 0.0f, 90.0f))
            {
                data.innerCutoffAngle = XMConvertToRadians(inner);
            }
            if (ImGui::SliderFloat(("OuterCutoff##" + std::to_string(i)).c_str(), &outer, inner, 90.0f))
            {
                data.outerCutoffAngle = XMConvertToRadians(outer);
            }
        }

        if (type != LightType::Directional)
        {
            ImGui::InputFloat(("Constant##" + std::to_string(i)).c_str(), &data.constant, 0.0f, 0.0f, "%.3f");
            ImGui::InputFloat(("Linear##" + std::to_string(i)).c_str(), &data.linear, 0.0f, 0.0f, "%.3f");
            ImGui::InputFloat(("Quadratic##" + std::to_string(i)).c_str(), &data.quadratic, 0.0f, 0.0f, "%.3f");
        }

        light->SetLightData(data);
    }
    ImGui::End();
}

void LightingManager::UploadLightingBuffer(Renderer* renderer)
{
    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    if (frameResource && frameResource->cbLighting)
    {
        frameResource->cbLighting->CopyData(0, lightingData);
    }
}

void LightingManager::UploadShadowViewProjBuffer(Renderer* renderer)
{
    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    if (!(frameResource && frameResource->cbShadowViewProj))
        return;

    CB_ShadowMapViewProj shadowData = {};

    // 전체 슬롯 인덱스
    UINT slot = 0;

    // 각 라이트별로 shadowViewProjMatrices를 꺼내서 채워넣는다.
    for (auto& lightPtr : lights)
    {
        if (!lightPtr->IsShadowCastingEnabled())
            continue;

        const auto& matrices = lightPtr->GetShadowViewProjMatrices();
        for (size_t j = 0; j < matrices.size() && slot < MaxShadowMaps; ++j)
        {
            // 행렬전치
            XMStoreFloat4x4(&shadowData.ShadowMapViewProj[slot], XMMatrixTranspose(matrices[j]));
            ++slot;
        }
    }

    frameResource->cbShadowViewProj->CopyData(0, shadowData);
}

const std::vector<std::shared_ptr<BaseLight>>& LightingManager::GetLights() const
{
    return lights;
}
