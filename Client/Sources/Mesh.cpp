#include "Mesh.h"

Mesh::Mesh() : vertexBuffer(nullptr), indexBuffer(nullptr), indexCount(0) {}

Mesh::~Mesh() {
}

bool Mesh::Initialize(ID3D11Device* device, const std::vector<MeshVertex>& vertices, const std::vector<unsigned int>& indices) {
    if (!CreateBuffers(device, vertices, indices)) {
        return false;
    }

    return true;
}

bool Mesh::CreateBuffers(ID3D11Device* device, const std::vector<MeshVertex>& vertices, const std::vector<unsigned int>& indices) {
    // 버텍스 버퍼 생성
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = UINT(sizeof(MeshVertex) * vertices.size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vinitData = {};
    vinitData.pSysMem = vertices.data();

    HRESULT hr = device->CreateBuffer(&vbd, &vinitData, vertexBuffer.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // 인덱스 버퍼 생성
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = UINT(sizeof(unsigned int) * indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = indices.data();

    hr = device->CreateBuffer(&ibd, &iinitData, indexBuffer.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    indexCount = static_cast<unsigned int>(indices.size());

    return true;
}

ID3D11Buffer* Mesh::GetVertexBuffer() const {
    return vertexBuffer.Get();
}

ID3D11Buffer* Mesh::GetIndexBuffer() const {
    return indexBuffer.Get();
}

unsigned int Mesh::GetIndexCount() const {
    return indexCount;
}

std::shared_ptr<Mesh> Mesh::CreateCube(ID3D11Device* device) {
    std::vector<MeshVertex> vertices = {
        // Top (+Y)
        {{-1,  1, -1}, {0, 1, 0}, {0, 0}},
        {{-1,  1,  1}, {0, 1, 0}, {1, 0}},
        {{ 1,  1,  1}, {0, 1, 0}, {1, 1}},
        {{ 1,  1, -1}, {0, 1, 0}, {0, 1}},

        // Bottom (-Y)
        {{-1, -1, -1}, {0, -1, 0}, {0, 0}},
        {{ 1, -1, -1}, {0, -1, 0}, {1, 0}},
        {{ 1, -1,  1}, {0, -1, 0}, {1, 1}},
        {{-1, -1,  1}, {0, -1, 0}, {0, 1}},

        // Front (-Z)
        {{-1, -1, -1}, {0, 0, -1}, {0, 0}},
        {{-1,  1, -1}, {0, 0, -1}, {1, 0}},
        {{ 1,  1, -1}, {0, 0, -1}, {1, 1}},
        {{ 1, -1, -1}, {0, 0, -1}, {0, 1}},

        // Back (+Z)
        {{-1, -1,  1}, {0, 0, 1}, {0, 0}},
        {{ 1, -1,  1}, {0, 0, 1}, {1, 0}},
        {{ 1,  1,  1}, {0, 0, 1}, {1, 1}},
        {{-1,  1,  1}, {0, 0, 1}, {0, 1}},

        // Left (-X)
        {{-1, -1,  1}, {-1, 0, 0}, {0, 0}},
        {{-1,  1,  1}, {-1, 0, 0}, {1, 0}},
        {{-1,  1, -1}, {-1, 0, 0}, {1, 1}},
        {{-1, -1, -1}, {-1, 0, 0}, {0, 1}},

        // Right (+X)
        {{ 1, -1, -1}, {1, 0, 0}, {0, 0}},
        {{ 1,  1, -1}, {1, 0, 0}, {1, 0}},
        {{ 1,  1,  1}, {1, 0, 0}, {1, 1}},
        {{ 1, -1,  1}, {1, 0, 0}, {0, 1}},
    };

    std::vector<unsigned int> indices = {
        // Top
         0,  1,  2,  0,  2,  3,
         // Bottom
          4,  5,  6,  4,  6,  7,
          // Front
           8,  9, 10,  8, 10, 11,
           // Back
           12, 13, 14, 12, 14, 15,
           // Left
           16, 17, 18, 16, 18, 19,
           // Right
           20, 21, 22, 20, 22, 23
    };

    auto mesh = std::make_shared<Mesh>();
    if (!mesh->Initialize(device, vertices, indices))
        return nullptr;

    return mesh;
}

std::shared_ptr<Mesh> Mesh::CreateQuad(ID3D11Device* device)
{
    std::vector<MeshVertex> vertices = {
        {{-1.0f, -1.0f, 0.0f}, {0, 0, -1}, {0.0f, 1.0f}}, // Bottom-left
        {{ 1.0f, -1.0f, 0.0f}, {0, 0, -1}, {1.0f, 1.0f}}, // Bottom-right
        {{-1.0f,  1.0f, 0.0f}, {0, 0, -1}, {0.0f, 0.0f}}, // Top-left
        {{ 1.0f,  1.0f, 0.0f}, {0, 0, -1}, {1.0f, 0.0f}}, // Top-right
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 1, 3
    };

    auto mesh = std::make_shared<Mesh>();
    if (!mesh->Initialize(device, vertices, indices))
        return nullptr;

    return mesh;
}