#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "PipelineStateDesc.h"

using Microsoft::WRL::ComPtr;

class Renderer;

class PipelineStateManager {
public:
    // 렌더러 포인터 하나만 넘겨받도록 변경
    explicit PipelineStateManager(Renderer* renderer_);
    ~PipelineStateManager();

    // 초기화 단계에서 삼각형 전용 PSO 생성
    bool InitializePSOs();

    // 필요 시 PSO를 생성하거나 기존 캐시 반환
    ID3D12PipelineState* GetOrCreate(const PipelineStateDesc& desc);

    // 캐시된 PSO 직접 조회
    ID3D12PipelineState* Get(const std::wstring& name) const;

    // 앱 종료 시 리소스 정리
    void Cleanup();

private:
    // 삼각형 전용 PSO 디스크립터 생성함수
    PipelineStateDesc CreateTrianglePSODesc() const;

    // Phong 쉐이딩 PSO 디스크립터 생성함수
    PipelineStateDesc CreatePhongPSODesc() const;

    // PSO 생성 내부 로직
    bool CreatePSO(const PipelineStateDesc& desc);

    Renderer* renderer;  // 렌더러 참조
    ID3D12Device* device;    // device = renderer->GetDevice()
    std::unordered_map<
        std::wstring,
        ComPtr<ID3D12PipelineState>
    > psoMap;  // 이름 → PSO 캐시
};