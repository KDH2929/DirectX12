#include "Mesh.h"
#include "Renderer.h"
#include <directx/d3dx12.h>
#include <format>

static void BuildCube(std::vector<MeshVertex>& outVertices,
    std::vector<uint32_t>& outIndices) {
    outVertices = {
        // +Y
        {{-1,  1, -1},{0,1,0},{0,0}}, {{-1,  1,  1},{0,1,0},{1,0}},
        {{ 1,  1,  1},{0,1,0},{1,1}}, {{ 1,  1, -1},{0,1,0},{0,1}},
        // -Y
        {{-1, -1, -1},{0,-1,0},{0,0}}, {{ 1, -1, -1},{0,-1,0},{1,0}},
        {{ 1, -1,  1},{0,-1,0},{1,1}}, {{-1, -1,  1},{0,-1,0},{0,1}},
        // -Z
        {{-1,-1,-1},{0,0,-1},{0,0}}, {{-1,1,-1},{0,0,-1},{1,0}},
        {{ 1,1,-1},{0,0,-1},{1,1}}, {{ 1,-1,-1},{0,0,-1},{0,1}},
        // +Z
        {{-1,-1,1},{0,0,1},{0,0}}, {{ 1,-1,1},{0,0,1},{1,0}},
        {{ 1,1,1},{0,0,1},{1,1}}, {{-1,1,1},{0,0,1},{0,1}},
        // -X
        {{-1,-1,1},{-1,0,0},{0,0}}, {{-1,1,1},{-1,0,0},{1,0}},
        {{-1,1,-1},{-1,0,0},{1,1}}, {{-1,-1,-1},{-1,0,0},{0,1}},
        // +X
        {{ 1,-1,-1},{1,0,0},{0,0}}, {{ 1,1,-1},{1,0,0},{1,0}},
        {{ 1,1,1},{1,0,0},{1,1}}, {{ 1,-1,1},{1,0,0},{0,1}}
    };
    outIndices = {
        0,1,2, 0,2,3,   4,5,6, 4,6,7,
        8,9,10,8,10,11, 12,13,14,12,14,15,
        16,17,18,16,18,19, 20,21,22,20,22,23
    };
}

static void BuildQuad(std::vector<MeshVertex>& outVertices,
    std::vector<uint32_t>& outIndices) {
    outVertices = {
        {{-1,-1,0},{0,0,-1},{0,1}}, {{ 1,-1,0},{0,0,-1},{1,1}},
        {{-1, 1,0},{0,0,-1},{0,0}}, {{ 1, 1,0},{0,0,-1},{1,0}}
    };
    outIndices = { 0,1,2, 2,1,3 };
}

bool Mesh::Initialize(Renderer* renderer,
    const std::vector<MeshVertex>& vertices,
    const std::vector<uint32_t>& indices) {
    if (vertices.empty() || indices.empty()) return false;

    indexCount = static_cast<uint32_t>(indices.size());

    const size_t vertexBytes = vertices.size() * sizeof(MeshVertex);
    const size_t indexBytes = indices.size() * sizeof(uint32_t);


    return UploadBuffers(renderer,
        vertices.data(), vertexBytes,
        indices.data(), indexBytes);
}

bool Mesh::UploadBuffers(Renderer* renderer,
    const void* vertexData, size_t vertexByteSize,
    const void* indexData, size_t indexByteSize) {
    ID3D12Device* device = renderer->GetDevice();
    if (!device) return false;

    // 1) Create DEFAULT-heap buffers (GPU-local)
    CD3DX12_HEAP_PROPERTIES defaultProps(D3D12_HEAP_TYPE_DEFAULT);

    CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexByteSize);
    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(indexByteSize);

    ThrowIfFailed(device->CreateCommittedResource(
        &defaultProps, D3D12_HEAP_FLAG_NONE,
        &vbDesc, D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&vertexBuffer)));

    ThrowIfFailed(device->CreateCommittedResource(
        &defaultProps, D3D12_HEAP_FLAG_NONE,
        &ibDesc, D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&indexBuffer)));

    // 2) Create UPLOAD-heap staging buffers
    CD3DX12_HEAP_PROPERTIES uploadProps(D3D12_HEAP_TYPE_UPLOAD);
    ComPtr<ID3D12Resource> vertexUpload;
    ComPtr<ID3D12Resource> indexUpload;

    ThrowIfFailed(device->CreateCommittedResource(
        &uploadProps, D3D12_HEAP_FLAG_NONE,
        &vbDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&vertexUpload)));

    ThrowIfFailed(device->CreateCommittedResource(
        &uploadProps, D3D12_HEAP_FLAG_NONE,
        &ibDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&indexUpload)));

    // 3) Copy CPU data into upload heap (memcpy)
    void* mapped = nullptr;
    CD3DX12_RANGE range(0, 0);
    ThrowIfFailed(vertexUpload->Map(0, &range, &mapped));
    memcpy(mapped, vertexData, vertexByteSize);
    vertexUpload->Unmap(0, nullptr);

    ThrowIfFailed(indexUpload->Map(0, &range, &mapped));
    memcpy(mapped, indexData, indexByteSize);
    indexUpload->Unmap(0, nullptr);

    // 4) Record copy commands on renderer’s COPY list
    ID3D12GraphicsCommandList* copyList = renderer->GetUploadCommandList();
    ID3D12CommandAllocator* copyAlloc = renderer->GetUploadCommandAllocator();

    copyAlloc->Reset();
    copyList->Reset(copyAlloc, nullptr);

    copyList->CopyResource(vertexBuffer.Get(), vertexUpload.Get());
    copyList->CopyResource(indexBuffer.Get(), indexUpload.Get());

    // 그래픽 상태로 전환하지 않고 COMMON 상태까지만 전환
    CD3DX12_RESOURCE_BARRIER barriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COMMON),
        CD3DX12_RESOURCE_BARRIER::Transition(
            indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COMMON)
    };
    copyList->ResourceBarrier(2, barriers);

    ThrowIfFailed(copyList->Close());

    ID3D12CommandQueue* copyQueue = renderer->GetUploadQueue();
    copyQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&copyList));

    // 5) Fence & wait so buffers are ready before first draw
    UINT64 fenceValue = renderer->IncrementUploadFenceValue();
    ID3D12Fence* fence = renderer->GetUploadFence();

    copyQueue->Signal(fence, fenceValue);
    if (fence->GetCompletedValue() < fenceValue) {
        HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);
        fence->SetEventOnCompletion(fenceValue, eventHandle);
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    // 6) Destroy upload buffers (implicit when ComPtr goes out of scope)

    // 7) Fill buffer views
    vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexView.SizeInBytes = static_cast<UINT>(vertexByteSize);
    vertexView.StrideInBytes = sizeof(MeshVertex);

    indexView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexView.SizeInBytes = static_cast<UINT>(indexByteSize);
    indexView.Format = DXGI_FORMAT_R32_UINT;

    return true;
}

std::shared_ptr<Mesh> Mesh::CreateCube(Renderer* renderer) {
    std::vector<MeshVertex> v; std::vector<uint32_t> i;
    BuildCube(v, i);
    auto mesh = std::make_shared<Mesh>();
    return mesh->Initialize(renderer, v, i) ? mesh : nullptr;
}

std::shared_ptr<Mesh> Mesh::CreateQuad(Renderer* renderer) {
    std::vector<MeshVertex> v; std::vector<uint32_t> i;
    BuildQuad(v, i);
    auto mesh = std::make_shared<Mesh>();
    return mesh->Initialize(renderer, v, i) ? mesh : nullptr;
}
