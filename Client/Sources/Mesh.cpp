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
        // Bottom face (-Y) ���� �ݴ��
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
    // �ܼ� 2D ���� - U�� +X
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

    // 1) ���ؽ� ����
    for (uint32_t lat = 0; lat <= latitudeSegments; ++lat) {
        float phi = float(lat) / latitudeSegments * XM_PI;           // 0 ~ ��
        float y = std::cos(phi);
        float r = std::sin(phi);
        for (uint32_t lon = 0; lon <= longitudeSegments; ++lon) {
            float theta = float(lon) / longitudeSegments * 2.0f * XM_PI; // 0 ~ 2��
            float x = r * std::cos(theta);
            float z = r * std::sin(theta);

            MeshVertex v;
            v.position = XMFLOAT3{ x, y, z };
            v.normal = XMFLOAT3{ x, y, z };  // ���� ���̹Ƿ� ��ġ ���� = ���
            v.texCoord = XMFLOAT2{
                float(lon) / longitudeSegments,
                float(lat) / latitudeSegments
            };
            // tangent = ��position/�ӥ� normalized
            XMFLOAT3 tan = { -r * std::sin(theta), 0.0f, r * std::cos(theta) };
            float invLen = 1.0f / std::sqrt(
                tan.x * tan.x + tan.y * tan.y + tan.z * tan.z);
            v.tangent = XMFLOAT3{ tan.x * invLen, tan.y * invLen, tan.z * invLen };

            outVertices.push_back(v);
        }
    }

    // 2) �ε��� ���� (�� �簢���� �� ���� �ﰢ������, CW ���� ����)
    for (uint32_t lat = 0; lat < latitudeSegments; ++lat) {
        for (uint32_t lon = 0; lon < longitudeSegments; ++lon) {
            uint32_t current = lat * (longitudeSegments + 1) + lon;
            uint32_t next = current + (longitudeSegments + 1);

            // �ﰢ��1: (current, current+1, next) �� CW ����
            outIndices.push_back(current);
            outIndices.push_back(current + 1);
            outIndices.push_back(next);

            // �ﰢ��2: (current+1, next+1, next) �� CW ����
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
    // 1) DEFAULT-heap ���� ���� (COMMON)
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

    // 2) UPLOAD-heap ������¡ ���� ����
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

    // 3) CPU �� UPLOAD ����
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

    // 4) Copy ���� Ŀ�ǵ� ����Ʈ�� ���� ��� ���
    auto* copyAlloc = renderer->GetCopyCommandAllocator();
    auto* copyList = renderer->GetCopyCommandList();

    THROW_IF_FAILED(copyAlloc->Reset());
    THROW_IF_FAILED(copyList->Reset(copyAlloc, nullptr));

    // 4-1) COMMON �� COPY_DEST
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

    // 4-2) ���� Copy
    copyList->CopyResource(vertexBuffer.Get(), vertexUpload.Get());
    copyList->CopyResource(indexBuffer.Get(), indexUpload.Get());

    // 4-3) COPY_DEST �� COMMON ���� ����
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

    // 5) Copy ť�� ���� & �Ϸ� ���
    ID3D12CommandList* lists[] = { copyList };
    renderer->GetCopyQueue()->ExecuteCommandLists(_countof(lists), lists);

    const UINT64 fenceValue = renderer->SignalCopyFence();
    renderer->WaitCopyFence(fenceValue);

    // 6) VB/IB �� ����
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
