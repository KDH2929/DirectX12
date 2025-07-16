#pragma once

#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;


// PSO ������ ��� ����ü
// ��� D3D12_GRAPHICS_PIPELINE_STATE_DESC �� ���� �Ȱ�����, ���� Ȯ�强�� ����ϸ� �ʿ����� ������ ������ ��.. 

struct PipelineStateDesc {
    std::wstring                          name;           // PSO �ĺ� Ű
    ComPtr<ID3DBlob>                      vsBlob;         // �����ϵ� VS Blob
    ComPtr<ID3DBlob>                      gsBlob;
    ComPtr<ID3DBlob>                      psBlob;         // �����ϵ� PS Blob
    ComPtr<ID3D12RootSignature>           rootSignature;      // �̹� ������ ��Ʈ �ñ״�ó
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;    // �Է� ���̾ƿ�
    D3D12_RASTERIZER_DESC                 rasterizerDesc = D3D12_RASTERIZER_DESC{};
    D3D12_BLEND_DESC                      blendDesc = D3D12_BLEND_DESC{};
    D3D12_DEPTH_STENCIL_DESC              depthStencilDesc = D3D12_DEPTH_STENCIL_DESC{};
    UINT                                  numRenderTargets = 1;
    DXGI_FORMAT                           rtvFormats[8] = { DXGI_FORMAT_R8G8B8A8_UNORM };
    DXGI_FORMAT                           dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE        topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    UINT                                  sampleCount = 1;
    UINT                      sampleMask = UINT_MAX;
    DXGI_SAMPLE_DESC         sampleDesc = { 1, 0 };
};