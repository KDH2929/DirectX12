#include "LightingManager.h"
#include <algorithm>
#include <cstring>
#include <directx/d3dx12.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

LightingManager::LightingManager(ID3D12Device* device)
{
    CreateConstantBuffer(device,
        sizeof(CB_Lighting),
        lightingBuffer, lightingMapped);

    CreateConstantBuffer(device,
        sizeof(CB_ShadowMapViewProj),
        shadowViewProjBuffer, shadowViewProjMapped);
}

void LightingManager::AddLight(std::shared_ptr<BaseLight> light)
{
    if (lights.size() < MAX_LIGHTS)
        lights.push_back(light);
}

void LightingManager::ClearLights()
{
    lights.clear();
}

void LightingManager::Update(Camera* camera)
{
    lightingData = {};
    lightingData.cameraWorld = camera->GetPosition();

    size_t lightCount = std::min<size_t>(lights.size(), static_cast<size_t>(MAX_LIGHTS));
    for (size_t i = 0; i < lightCount; ++i)
    {
        lightingData.lights[i] = lights[i]->GetLightData();

        if (lights[i]->IsShadowCastingEnabled()) {
            lights[i]->Update(camera); // ShadowViewProj 행렬 갱신
        }
    }

    UpdateImGui();
    WriteLightingBuffer();

    std::vector<XMMATRIX> allShadowViewProj;
    allShadowViewProj.reserve(MAX_SHADOW_DSV_COUNT);

    for (size_t i = 0; i < lightCount; ++i)
    {
        auto& light = lights[i];
        if (!light->IsShadowCastingEnabled())
            continue;

        const auto& viewProjMatrices = light->GetShadowViewProjMatrices();
        for (const XMMATRIX& viewProj : viewProjMatrices)
        {
            // 전치한 뒤 벡터에 추가
            allShadowViewProj.push_back(XMMatrixTranspose(viewProj));
        }
    }

    WriteShadowViewProjBuffer(allShadowViewProj);
}

void LightingManager::UpdateImGui()
{
    ImGui::Begin("Lighting Controls");

    for (size_t i = 0; i < lights.size(); ++i)
    {
        auto& light = lights[i];
        LightType type = light->GetType();
        LightData data = light->GetLightData();

        ImGui::Separator();
        ImGui::Text("%s Light",
            type == LightType::Directional ? "Directional" :
            type == LightType::Point ? "Point" :
            type == LightType::Spot ? "Spot" : "Unknown");

        // Color & Strength
        ImGui::ColorEdit3(("Color##" + std::to_string(i)).c_str(), reinterpret_cast<float*>(&data.color));
        ImGui::SliderFloat(("Strength##" + std::to_string(i)).c_str(), &data.strength, 0.0f, 10.0f);

        // Position for Point & Spot
        if (type != LightType::Directional)
        {
            ImGui::DragFloat3(("Position##" + std::to_string(i)).c_str(),
                reinterpret_cast<float*>(&data.position),
                0.1f, -50.0f, 50.0f);
        }

        // Direction for Directional & Spot
        if (type != LightType::Point)
        {
            ImGui::DragFloat3(("Direction##" + std::to_string(i)).c_str(),
                reinterpret_cast<float*>(&data.direction),
                0.01f, -1.0f, 1.0f);
        }

        // Spot: Inner/Outer cutoff angles (in degrees)
        if (type == LightType::Spot)
        {
            float innerDeg = XMConvertToDegrees(data.innerCutoffAngle);
            float outerDeg = XMConvertToDegrees(data.outerCutoffAngle);
            ImGui::SliderFloat(("Inner Cutoff##" + std::to_string(i)).c_str(), &innerDeg, 0.0f, 90.0f);
            ImGui::SliderFloat(("Outer Cutoff##" + std::to_string(i)).c_str(), &outerDeg, innerDeg, 90.0f);
            data.innerCutoffAngle = XMConvertToRadians(innerDeg);
            data.outerCutoffAngle = XMConvertToRadians(outerDeg);
        }

        // Attenuation constants for Point & Spot
        if (type != LightType::Directional)
        {
            ImGui::InputFloat(("Constant##" + std::to_string(i)).c_str(), &data.constant, 0.0f, 0.0f, "%.3f");
            ImGui::InputFloat(("Linear##" + std::to_string(i)).c_str(), &data.linear, 0.0f, 0.0f, "%.3f");
            ImGui::InputFloat(("Quadratic##" + std::to_string(i)).c_str(), &data.quadratic, 0.0f, 0.0f, "%.3f");
        }

        // Type
        ImGui::Text("Type: %s",
            type == LightType::Directional ? "Directional" :
            type == LightType::Point ? "Point" :
            type == LightType::Spot ? "Spot" : "Unknown");

        light->SetLightData(data);
    }

    ImGui::End();
}

void LightingManager::WriteLightingBuffer()
{
    // CB_Lighting GPU 업로드
    std::memcpy(lightingMapped, &lightingData, sizeof(CB_Lighting));
}

void LightingManager::WriteShadowViewProjBuffer(const std::vector<XMMATRIX>& shadowViewProj)
{
    // CB_ShadowMapViewProj GPU 업로드
    CB_ShadowMapViewProj cb{};
    for (int i = 0; i < MAX_SHADOW_DSV_COUNT; ++i)
    {
        XMStoreFloat4x4(
            &cb.ShadowMapViewProj[i],
            XMMatrixTranspose(shadowViewProj[i]));
    }
    std::memcpy(shadowViewProjMapped, &cb, sizeof(cb));
}

const CB_Lighting& LightingManager::GetLightingData() const
{
    return lightingData;
}

const std::vector<std::shared_ptr<BaseLight>>& LightingManager::GetLights() const
{
    return lights;
}

ID3D12Resource* LightingManager::GetLightingCB() const
{
    return lightingBuffer.Get();
}

ID3D12Resource* LightingManager::GetShadowViewProjCB() const
{
    return shadowViewProjBuffer.Get();
}

void LightingManager::CreateConstantBuffer(ID3D12Device* device, UINT64 size, Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, UINT8*& mappedPtr)
{
    // 256바이트 경계 정렬
    UINT64 alignedSize = (size + 255) & ~255ull;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize);
    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&buffer));

    CD3DX12_RANGE readRange(0, 0);
    buffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedPtr));
}
