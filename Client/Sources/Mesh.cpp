#include "Mesh.h"
#include "Renderer.h"
#include <directx/d3dx12.h>
#include <format>

static void BuildCube(std::vector<MeshVertex>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    outVertices = {
        // +Y (Top)
        {{-1, 1,-1},{ 0, 1, 0},{0,0},{0,0,1}},
        {{-1, 1, 1},{ 0, 1, 0},{1,0},{0,0,1}},
        {{ 1, 1, 1},{ 0, 1, 0},{1,1},{0,0,1}},
        {{ 1, 1,-1},{ 0, 1, 0},{0,1},{0,0,1}},

        // -Y (Bottom)
        {{-1,-1,-1},{ 0,-1, 0},{0,0},{1,0,0}},
        {{ 1,-1,-1},{ 0,-1, 0},{1,0},{1,0,0}},
        {{ 1,-1, 1},{ 0,-1, 0},{1,1},{1,0,0}},
        {{-1,-1, 1},{ 0,-1, 0},{0,1},{1,0,0}},

        // -Z (Back)
        {{-1,-1,-1},{ 0, 0,-1},{0,0},{0,1,0}},
        {{-1, 1,-1},{ 0, 0,-1},{1,0},{0,1,0}},
        {{ 1, 1,-1},{ 0, 0,-1},{1,1},{0,1,0}},
        {{ 1,-1,-1},{ 0, 0,-1},{0,1},{0,1,0}},

        // +Z (Front)
        {{-1,-1, 1},{ 0, 0, 1},{0,0},{1,0,0}},
        {{ 1,-1, 1},{ 0, 0, 1},{1,0},{1,0,0}},
        {{ 1, 1, 1},{ 0, 0, 1},{1,1},{1,0,0}},
        {{-1, 1, 1},{ 0, 0, 1},{0,1},{1,0,0}},

        // -X (Left)
        {{-1,-1, 1},{-1, 0, 0},{0,0},{0,1,0}},
        {{-1, 1, 1},{-1, 0, 0},{1,0},{0,1,0}},
        {{-1, 1,-1},{-1, 0, 0},{1,1},{0,1,0}},
        {{-1,-1,-1},{-1, 0, 0},{0,1},{0,1,0}},

        // +X (Right)
        {{ 1,-1,-1},{ 1, 0, 0},{0,0},{0,1,0}},
        {{ 1, 1,-1},{ 1, 0, 0},{1,0},{0,1,0}},
        {{ 1, 1, 1},{ 1, 0, 0},{1,1},{0,1,0}},
        {{ 1,-1, 1},{ 1, 0, 0},{0,1},{0,1,0}}
    };

    // Top face (+Y)
    outIndices = {
        // Top face (+Y)
        0, 1, 2,   0, 2, 3,
        // Bottom face (-Y) 역시 반대로
        4, 5, 6,   4, 6, 7,
        // Back face (-Z)
        8, 9,10,   8,10,11,
        // Front face (+Z)
        12,13,14,  12,14,15,
        // Left face (-X)
        16,17,18,  16,18,19,
        // Right face (+X)
        20,21,22,  20,22,23
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
    outIndices = {
        0, 2, 1,
        2, 3, 1
    };
}

static void BuildSphere(
    uint32_t latitudeSegments,
    uint32_t longitudeSegments,
    std::vector<MeshVertex>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    outVertices.clear();
    outIndices.clear();

    // 1) 버텍스 생성
    for (uint32_t lat = 0; lat <= latitudeSegments; ++lat) {
        float phi = float(lat) / latitudeSegments * XM_PI;           // 0 ~ π
        float y = std::cos(phi);
        float r = std::sin(phi);
        for (uint32_t lon = 0; lon <= longitudeSegments; ++lon) {
            float theta = float(lon) / longitudeSegments * 2.0f * XM_PI; // 0 ~ 2π
            float x = r * std::cos(theta);
            float z = r * std::sin(theta);

            MeshVertex v;
            v.position = XMFLOAT3{ x, y, z };
            v.normal = XMFLOAT3{ x, y, z };  // 단위 구이므로 위치 벡터 = 노멀
            v.texCoord = XMFLOAT2{
                float(lon) / longitudeSegments,
                float(lat) / latitudeSegments
            };
            // tangent = ∂position/∂θ normalized
            XMFLOAT3 tan = { -r * std::sin(theta), 0.0f, r * std::cos(theta) };
            float invLen = 1.0f / std::sqrt(
                tan.x * tan.x + tan.y * tan.y + tan.z * tan.z);
            v.tangent = XMFLOAT3{ tan.x * invLen, tan.y * invLen, tan.z * invLen };

            outVertices.push_back(v);
        }
    }

    // 2) 인덱스 생성 (각 사각형을 두 개의 삼각형으로, CW 전면 기준)
    for (uint32_t lat = 0; lat < latitudeSegments; ++lat) {
        for (uint32_t lon = 0; lon < longitudeSegments; ++lon) {
            uint32_t current = lat * (longitudeSegments + 1) + lon;
            uint32_t next = current + (longitudeSegments + 1);

            // 삼각형1: (current, current+1, next) → CW 순서
            outIndices.push_back(current);
            outIndices.push_back(current + 1);
            outIndices.push_back(next);

            // 삼각형2: (current+1, next+1, next) → CW 순서
            outIndices.push_back(current + 1);
            outIndices.push_back(next + 1);
            outIndices.push_back(next);
        }
    }
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

    THROW_IF_FAILED(device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(&vertexBuffer)));

    THROW_IF_FAILED(device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(&indexBuffer)));

    // 2) UPLOAD-heap 스테이징 버퍼 생성
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    ComPtr<ID3D12Resource> vertexUpload, indexUpload;

    THROW_IF_FAILED(device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&vertexUpload)));

    THROW_IF_FAILED(device->CreateCommittedResource(
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

    THROW_IF_FAILED(copyAlloc->Reset());
    THROW_IF_FAILED(copyList->Reset(copyAlloc, nullptr));

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

    THROW_IF_FAILED(copyList->Close());

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

std::shared_ptr<Mesh> Mesh::CreateSphere(Renderer* renderer, uint32_t latitudeSegments, uint32_t longitudeSegments)
{
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t>   indices;
    BuildSphere(latitudeSegments, longitudeSegments, vertices, indices);

    auto mesh = std::make_shared<Mesh>();
    return mesh->Initialize(renderer, vertices, indices) ? mesh : nullptr;
}
