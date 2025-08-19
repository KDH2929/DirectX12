#pragma once

#include <memory>
#include <string>
#include <d3d12.h>
#include "Texture.h"


class Renderer;

// EnvironmentMaps: IBL용 환경맵 리소스 관리
class EnvironmentMaps
{
public:
    // 지정된 경로에서 IBL 리소스 로드
    // 경로가 유효하면 true 반환
    bool Load(
        Renderer* renderer,
        const std::wstring& irradianceMapPath,
        const std::wstring& specularMapPath,
        const std::wstring& brdfLutPath);
    

    // 명시된 루트 인덱스에 SRV 바인딩
    void Bind(
        ID3D12GraphicsCommandList* commandList,
        UINT rootIndexIrradiance,
        UINT rootIndexSpecular,
        UINT rootIndexBrdfLut) const;

private:
    std::shared_ptr<Texture> irradianceMap;      // 확산 IBL용 큐브맵
    std::shared_ptr<Texture> specularMap;        // 스펙큘러 프리필터 큐브맵
    std::shared_ptr<Texture> brdfLutTexture;     // BRDF 통합 LUT
};
