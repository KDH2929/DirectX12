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
    // ������ ������ �ϳ��� �Ѱܹ޵��� ����
    explicit PipelineStateManager(Renderer* renderer_);
    ~PipelineStateManager();

    // �ʱ�ȭ �ܰ迡�� �ﰢ�� ���� PSO ����
    bool InitializePSOs();

    // �ʿ� �� PSO�� �����ϰų� ���� ĳ�� ��ȯ
    ID3D12PipelineState* GetOrCreate(const PipelineStateDesc& desc);

    // ĳ�õ� PSO ���� ��ȸ
    ID3D12PipelineState* Get(const std::wstring& name) const;

    // �� ���� �� ���ҽ� ����
    void Cleanup();

private:
    // �ﰢ�� ���� PSO ��ũ���� �����Լ�
    PipelineStateDesc CreateTrianglePSODesc() const;

    // Phong ���̵� PSO ��ũ���� �����Լ�
    PipelineStateDesc CreatePhongPSODesc() const;

    // PSO ���� ���� ����
    bool CreatePSO(const PipelineStateDesc& desc);

    Renderer* renderer;  // ������ ����
    ID3D12Device* device;    // device = renderer->GetDevice()
    std::unordered_map<
        std::wstring,
        ComPtr<ID3D12PipelineState>
    > psoMap;  // �̸� �� PSO ĳ��
};