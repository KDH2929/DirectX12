#include "Mesh.h"
#include "Renderer.h"
#include <directx/d3dx12.h>
#include <format>

static void BuildCube(std::vector<MeshVertex>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    outVertices = {
        // +Y (Top) --- U는 +Z
        {{-1, 1,-1},{ 0, 1, 0},{0,0},{0,0,1}},
        {{-1, 1, 1},{ 0, 1, 0},{1,0},{0,0,1}},
        {{ 1, 1, 1},{ 0, 1, 0},{1,1},{0,0,1}},
        {{ 1, 1,-1},{ 0, 1, 0},{0,1},{0,0,1}},

        // -Y (Bottom) --- U는 +X
        {{-1,-1,-1},{ 0,-1, 0},{0,0},{1,0,0}},
        {{ 1,-1,-1},{ 0,-1, 0},{1,0},{1,0,0}},
        {{ 1,-1, 1},{ 0,-1, 0},{1,1},{1,0,0}},
        {{-1,-1, 1},{ 0,-1, 0},{0,1},{1,0,0}},

        // -Z (Back) --- U는 +Y
        {{-1,-1,-1},{ 0, 0,-1},{0,0},{0,1,0}},
        {{-1, 1,-1},{ 0, 0,-1},{1,0},{0,1,0}},
        {{ 1, 1,-1},{ 0, 0,-1},{1,1},{0,1,0}},
        {{ 1,-1,-1},{ 0, 0,-1},{0,1},{0,1,0}},

        // +Z (Front) --- U는 +X
        {{-1,-1, 1},{ 0, 0, 1},{0,0},{1,0,0}},
        {{ 1,-1, 1},{ 0, 0, 1},{1,0},{1,0,0}},
        {{ 1, 1, 1},{ 0, 0, 1},{1,1},{1,0,0}},
        {{-1, 1, 1},{ 0, 0, 1},{0,1},{1,0,0}},

        // -X (Left) --- U는 +Y
        {{-1,-1, 1},{-1, 0, 0},{0,0},{0,1,0}},
        {{-1, 1, 1},{-1, 0, 0},{1,0},{0,1,0}},
        {{-1, 1,-1},{-1, 0, 0},{1,1},{0,1,0}},
        {{-1,-1,-1},{-1, 0, 0},{0,1},{0,1,0}},

        // +X (Right) --- U는 +Y
        {{ 1,-1,-1},{ 1, 0, 0},{0,0},{0,1,0}},
        {{ 1, 1,-1},{ 1, 0, 0},{1,0},{0,1,0}},
        {{ 1, 1, 1},{ 1, 0, 0},{1,1},{0,1,0}},
        {{ 1,-1, 1},{ 1, 0, 0},{0,1},{0,1,0}}
    };

    outIndices = {
        0,1,2, 0,2,3,    4,5,6, 4,6,7,
        8,9,10, 8,10,11, 12,13,14, 12,14,15,
        16,17,18, 16,18,19, 20,21,22, 20,22,23
    };
}

static void BuildQuad(std::vector<MeshVertex>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    // 단순 2D 쿼드 - U는 +X
    outVertices = {
        {{-1,-1,0},{0,0,-1},{0,1},{1,0,0}},
        {{ 1,-1,0},{0,0,-1},{1,1},{1,0,0}},
        {{-1, 1,0},{0,0,-1},{0,0},{1,0,0}},
        {{ 1, 1,0},{0,0,-1},{1,0},{1,0,0}}
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
    const void* indexData, size_t indexByteSize)
{
    // 1) DEFAULT-heap 버퍼 생성 (COMMON)
    ID3D12Device* device = renderer->GetDevice();
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC     vbDesc = CD3DX12_RESOURCE_DESC::Buffer(UINT(vertexByteSize));
    CD3DX12_RESOURCE_DESC     ibDesc = CD3DX12_RESOURCE_DESC::Buffer(UINT(indexByteSize));

    CHECK_HR(device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(&vertexBuffer)));

    CHECK_HR(device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(&indexBuffer)));

    // 2) UPLOAD-heap 스테이징 버퍼 생성
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    ComPtr<ID3D12Resource> vertexUpload, indexUpload;

    CHECK_HR(device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&vertexUpload)));

    CHECK_HR(device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&indexUpload)));

    // 3) CPU → UPLOAD 복사
    {
        void* mapped = nullptr;
        CD3DX12_RANGE readRange(0, 0);

        vertexUpload->Map(0, &readRange, &mapped);
        memcpy(mapped, vertexData, vertexByteSize);
        vertexUpload->Unmap(0, nullptr);

        indexUpload->Map(0, &readRange, &mapped);
        memcpy(mapped, indexData, indexByteSize);
        indexUpload->Unmap(0, nullptr);
    }

    // 4) Copy 전용 커맨드 리스트에 복사 명령 기록
    auto* copyAlloc = renderer->GetCopyCommandAllocator();
    auto* copyList = renderer->GetCopyCommandList();

    CHECK_HR(copyAlloc->Reset());
    CHECK_HR(copyList->Reset(copyAlloc, nullptr));

    // 4-1) COMMON → COPY_DEST
    {
        D3D12_RESOURCE_BARRIER barriers[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(
                vertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST),
            CD3DX12_RESOURCE_BARRIER::Transition(
                indexBuffer.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST)
        };
        copyList->ResourceBarrier(_countof(barriers), barriers);
    }

    // 4-2) 실제 Copy
    copyList->CopyResource(vertexBuffer.Get(), vertexUpload.Get());
    copyList->CopyResource(indexBuffer.Get(), indexUpload.Get());

    // 4-3) COPY_DEST → COMMON 으로 복구
    {
        D3D12_RESOURCE_BARRIER barriers[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(
                vertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_COMMON),
            CD3DX12_RESOURCE_BARRIER::Transition(
                indexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_COMMON)
        };
        copyList->ResourceBarrier(_countof(barriers), barriers);
    }

    CHECK_HR(copyList->Close());

    // 5) Copy 큐에 제출 & 완료 대기
    ID3D12CommandList* lists[] = { copyList };
    renderer->GetCopyQueue()->ExecuteCommandLists(_countof(lists), lists);

    const UINT64 fenceValue = renderer->SignalCopyFence();
    renderer->WaitCopyFence(fenceValue);

    // 6) VB/IB 뷰 설정
    vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexView.SizeInBytes = UINT(vertexByteSize);
    vertexView.StrideInBytes = sizeof(MeshVertex);

    indexView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexView.SizeInBytes = UINT(indexByteSize);
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
