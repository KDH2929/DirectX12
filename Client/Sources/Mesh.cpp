#include "Mesh.h"
#include "Renderer.h"
#include <directx/d3dx12.h>

Mesh::Mesh() = default;
Mesh::~Mesh() = default;


void Mesh::CopyDataToGpuBuffer(ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    const void* sourceDataPtr_,
    UINT                       sizeBytes_,
    ComPtr<ID3D12Resource>& uploadBufferOut_,
    ComPtr<ID3D12Resource>& gpuBufferOut_,
    D3D12_RESOURCE_STATES      finalResourceState_)
{
    CD3DX12_HEAP_PROPERTIES uploadProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC   resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeBytes_);

    // Upload heap buffer
    device->CreateCommittedResource(&uploadProperties, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&uploadBufferOut_));

    void* mappedPtr = nullptr;
    uploadBufferOut_->Map(0, nullptr, &mappedPtr);
    memcpy(mappedPtr, sourceDataPtr_, sizeBytes_);
    uploadBufferOut_->Unmap(0, nullptr);

    // Default heap buffer
    CD3DX12_HEAP_PROPERTIES defaultProperties(D3D12_HEAP_TYPE_DEFAULT);
    device->CreateCommittedResource(&defaultProperties, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&gpuBufferOut_));

    // Schedule copy + state transition
    commandList->CopyResource(gpuBufferOut_.Get(), uploadBufferOut_.Get());
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        gpuBufferOut_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, finalResourceState_);
    commandList->ResourceBarrier(1, &barrier);
}

// ---------------------------------------------------------------------------
bool Mesh::CreateGpuBuffers(Renderer* renderer,
    const void* vertexDataPtr_, UINT vertexBytes_,
    const void* indexDataPtr_, UINT indexBytes_)
{
    ID3D12Device* device = renderer->GetDevice();
    auto          uploadAllocator = renderer->GetUploadCommandAllocator();
    auto          uploadList = renderer->GetUploadCommandList();

    uploadAllocator->Reset();
    uploadList->Reset(uploadAllocator, nullptr);

    ComPtr<ID3D12Resource> vertexUpload;
    ComPtr<ID3D12Resource> indexUpload;

    CopyDataToGpuBuffer(device, uploadList, vertexDataPtr_, vertexBytes_,
        vertexUpload, vertexBuffer,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    CopyDataToGpuBuffer(device, uploadList, indexDataPtr_, indexBytes_,
        indexUpload, indexBuffer,
        D3D12_RESOURCE_STATE_INDEX_BUFFER);

    uploadList->Close();
    ID3D12CommandList* lists[] = { uploadList };
    renderer->GetCommandQueue()->ExecuteCommandLists(1, lists);
    renderer->WaitForPreviousFrame();

    // Build views
    vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexView.StrideInBytes = sizeof(MeshVertex);
    vertexView.SizeInBytes = vertexBytes_;

    indexView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexView.Format = DXGI_FORMAT_R32_UINT;
    indexView.SizeInBytes = indexBytes_;

    return true;
}

// ---------------------------------------------------------------------------
bool Mesh::Initialize(Renderer* renderer,
    const std::vector<MeshVertex>& vertexVector,
    const std::vector<uint32_t>& indexVector)
{
    indexCount = static_cast<uint32_t>(indexVector.size());
    return CreateGpuBuffers(renderer,
        vertexVector.data(), UINT(vertexVector.size() * sizeof(MeshVertex)),
        indexVector.data(), UINT(indexVector.size() * sizeof(uint32_t)));
}

// ---------------------------------------------------------------------------
const D3D12_VERTEX_BUFFER_VIEW& Mesh::GetVertexBufferView() const { return vertexView; }
const D3D12_INDEX_BUFFER_VIEW& Mesh::GetIndexBufferView()  const { return indexView; }
uint32_t Mesh::GetIndexCount()  const { return indexCount; }



static void BuildCube(std::vector<MeshVertex>& verticesOut, std::vector<uint32_t>& indicesOut)
{
    verticesOut = {
        // +Y
        {{-1,  1, -1},{0,1,0},{0,0}}, {{-1,  1,  1},{0,1,0},{1,0}},
        {{ 1,  1,  1},{0,1,0},{1,1}}, {{ 1,  1, -1},{0,1,0},{0,1}},
        // -Y
        {{-1, -1, -1},{0,-1,0},{0,0}}, {{ 1, -1, -1},{0,-1,0},{1,0}},
        {{ 1, -1,  1},{0,-1,0},{1,1}}, {{-1, -1,  1},{0,-1,0},{0,1}},
        // -Z
        {{-1,-1,-1},{0,0,-1},{0,0}}, {{-1,1,-1},{0,0,-1},{1,0}},
        {{1,1,-1},{0,0,-1},{1,1}}, {{1,-1,-1},{0,0,-1},{0,1}},
        // +Z
        {{-1,-1,1},{0,0,1},{0,0}}, {{1,-1,1},{0,0,1},{1,0}},
        {{1,1,1},{0,0,1},{1,1}}, {{-1,1,1},{0,0,1},{0,1}},
        // -X
        {{-1,-1,1},{-1,0,0},{0,0}}, {{-1,1,1},{-1,0,0},{1,0}},
        {{-1,1,-1},{-1,0,0},{1,1}}, {{-1,-1,-1},{-1,0,0},{0,1}},
        // +X
        {{1,-1,-1},{1,0,0},{0,0}}, {{1,1,-1},{1,0,0},{1,0}},
        {{1,1,1},{1,0,0},{1,1}}, {{1,-1,1},{1,0,0},{0,1}}
    };
    indicesOut = {
        0,1,2, 0,2,3, 4,5,6, 4,6,7,
        8,9,10, 8,10,11, 12,13,14, 12,14,15,
        16,17,18, 16,18,19, 20,21,22, 20,22,23
    };
}

std::shared_ptr<Mesh> Mesh::CreateCube(Renderer* renderer)
{
    std::vector<MeshVertex> vertices; std::vector<uint32_t> indices;
    BuildCube(vertices, indices);
    auto meshPtr = std::make_shared<Mesh>();
    if (!meshPtr->Initialize(renderer, vertices, indices)) return nullptr;
    return meshPtr;
}


std::shared_ptr<Mesh> Mesh::CreateQuad(Renderer* renderer)
{
    std::vector<MeshVertex> vertices = {
        {{-1,-1,0},{0,0,-1},{0,1}}, {{1,-1,0},{0,0,-1},{1,1}},
        {{-1, 1,0},{0,0,-1},{0,0}}, {{1, 1,0},{0,0,-1},{1,0}}
    };
    std::vector<uint32_t> indices = { 0,1,2, 2,1,3 };
    auto meshPtr = std::make_shared<Mesh>();
    if (!meshPtr->Initialize(renderer, vertices, indices)) return nullptr;
    return meshPtr;
}
