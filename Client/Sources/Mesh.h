#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include <DirectXMath.h>

class Renderer;

using namespace DirectX;
using Microsoft::WRL::ComPtr;


struct MeshVertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 texCoordinates;
};


// Mesh --- holds GPU vertex/index buffers and their views
class Mesh
{
public:
    Mesh();
    ~Mesh();

    // Initialise from raw vertex/index data using the renderer's upload list.
    bool Initialize(Renderer* renderer,
        const std::vector<MeshVertex>& vertexVector,
        const std::vector<uint32_t>& indexVector);


    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const;
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView()  const;
    uint32_t                        GetIndexCount()       const;



    static std::shared_ptr<Mesh> CreateCube(Renderer* renderer);
    static std::shared_ptr<Mesh> CreateQuad(Renderer* renderer);

private:

    bool CreateGpuBuffers(Renderer* renderer,
        const void* vertexDataPtr_, UINT vertexBytes_,
        const void* indexDataPtr_, UINT indexBytes_);

    void CopyDataToGpuBuffer(ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const void* sourceDataPtr_,
        UINT                       sizeBytes_,
        ComPtr<ID3D12Resource>& uploadBufferOut_,
        ComPtr<ID3D12Resource>& gpuBufferOut_,
        D3D12_RESOURCE_STATES      finalResourceState_);

private:
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;

    D3D12_VERTEX_BUFFER_VIEW vertexView{};
    D3D12_INDEX_BUFFER_VIEW  indexView{};

    uint32_t indexCount = 0;
};