#include "LightingManager.h"
#include <algorithm>
#include <cstring>
#include <directx/d3dx12.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

LightingManager::LightingManager(ID3D12Device* device)
{
    CreateConstantBuffer(device);
}

void LightingManager::CreateConstantBuffer(ID3D12Device* device)
{
    // 256의 배수로 맞추기
    UINT bufferSize = (sizeof(CB_Lighting) + 255) & ~255u;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constantBuffer));

    CD3DX12_RANGE readRange(0, 0); // 읽지 않음
    constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedPointer));
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

void LightingManager::Update(const XMFLOAT3& cameraPosition)
{
    lightingData = {};
    lightingData.cameraWorld = cameraPosition;

    size_t count = std::min<size_t>(lights.size(), static_cast<size_t>(MAX_LIGHTS));
    for (size_t i = 0; i < count; ++i)
    {
        lightingData.lights[i] = lights[i]->GetLightData();

        if (lights[i]->IsShadowCastingEnabled()) {
            lights[i]->Update(cameraPosition); // ShadowViewProj 행렬 갱신
        }
    }
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

        ImGui::ColorEdit3(("Color##" + std::to_string(i)).c_str(), reinterpret_cast<float*>(&data.color));
        ImGui::SliderFloat(("Strength##" + std::to_string(i)).c_str(), &data.strength.x, 0.0f, 10.0f);
        data.strength.y = data.strength.z = data.strength.x;

        if (type != LightType::Directional)
        {
            ImGui::DragFloat3(("Position##" + std::to_string(i)).c_str(), reinterpret_cast<float*>(&data.position), 0.1f, -50.0f, 50.0f);
            ImGui::SliderFloat(("FalloffStart##" + std::to_string(i)).c_str(), &data.falloffStart, 0.0f, 50.0f);
            ImGui::SliderFloat(("FalloffEnd##" + std::to_string(i)).c_str(), &data.falloffEnd, 0.1f, 100.0f);
        }

        if (type != LightType::Point)
        {
            ImGui::DragFloat3(("Direction##" + std::to_string(i)).c_str(), reinterpret_cast<float*>(&data.direction), 0.01f, -1.0f, 1.0f);
        }

        ImGui::SliderFloat(("SpecPower##" + std::to_string(i)).c_str(), &data.specPower, 1.0f, 128.0f);

        light->SetLightData(data);
    }

    ImGui::End();
}

void LightingManager::WriteToConstantBuffer()
{
    std::memcpy(mappedPointer, &lightingData, sizeof(CB_Lighting));
}

ID3D12Resource* LightingManager::GetConstantBuffer() const
{
    return constantBuffer.Get();
}

const CB_Lighting& LightingManager::GetLightingData() const
{
    return lightingData;
}

const std::vector<std::shared_ptr<BaseLight>>& LightingManager::GetLights() const
{
    return lights;
}
