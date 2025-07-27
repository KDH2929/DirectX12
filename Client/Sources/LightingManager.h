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

class LightingManager
{
public:
    LightingManager(ID3D12Device* device);

    void AddLight(std::shared_ptr<BaseLight> light);
    void ClearLights();

    void Update(Camera* camera);
    void UpdateImGui();
    void WriteLightingBuffer();
    void WriteShadowViewProjBuffer(const std::vector<XMMATRIX>& shadowViewProj);

    const CB_Lighting& GetLightingData() const;
    const std::vector<std::shared_ptr<BaseLight>>& GetLights() const;

    ID3D12Resource* GetLightingCB() const;
    ID3D12Resource* GetShadowViewProjCB() const;

private:
    void CreateConstantBuffer(ID3D12Device* device,
        UINT64 size,
        Microsoft::WRL::ComPtr<ID3D12Resource>& buffer,
        UINT8*& mappedPtr);

    std::vector<std::shared_ptr<BaseLight>> lights;

    // CB_Lighting data & buffer
    CB_Lighting                         lightingData = {};
    Microsoft::WRL::ComPtr<ID3D12Resource> lightingBuffer;
    UINT8* lightingMapped = nullptr;

    // CB_ShadowMapViewProj buffer
    Microsoft::WRL::ComPtr<ID3D12Resource> shadowViewProjBuffer;
    UINT8* shadowViewProjMapped = nullptr;
};