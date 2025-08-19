#pragma once

#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;


// PSO 설정을 담는 구조체
// 사실 D3D12_GRAPHICS_PIPELINE_STATE_DESC 와 거의 똑같은데, 추후 확장성을 고려하면 필요하지 않을까 생각해 봄.. 

struct PipelineStateDesc {
    std::wstring                          name;           // PSO 식별 키
    ComPtr<ID3DBlob>                      vsBlob;         // 컴파일된 VS Blob
    ComPtr<ID3DBlob>                      gsBlob;
    ComPtr<ID3DBlob>                      psBlob;         // 컴파일된 PS Blob
    ComPtr<ID3D12RootSignature>           rootSignature;      // 이미 생성된 루트 시그니처
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;    // 입력 레이아웃
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