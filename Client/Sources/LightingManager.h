#pragma once

#include <vector>
#include <memory>
#include <wrl/client.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include "Lights/BaseLight.h"
#include "Lights/LightData.h"
#include "ConstantBuffers.h"

using namespace DirectX;

class LightingManager
{
public:
    LightingManager(ID3D12Device* device);

    void AddLight(std::shared_ptr<BaseLight> light);
    void ClearLights();

    void Update(const XMFLOAT3& cameraPosition);
    void UpdateImGui();
    void WriteToConstantBuffer();

    ID3D12Resource* GetConstantBuffer() const;
    const CB_Lighting& GetLightingData() const;
    const std::vector<std::shared_ptr<BaseLight>>& GetLights() const;

private:
    void CreateConstantBuffer(ID3D12Device* device);

    std::vector<std::shared_ptr<BaseLight>> lights;

    // CB_Lighting 에서 LightData 배열을 처리
    CB_Lighting lightingData = {};

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer;
    UINT8* mappedPointer = nullptr;
};