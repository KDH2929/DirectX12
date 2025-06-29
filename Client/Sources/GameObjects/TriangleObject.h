#pragma once

#include "GameObject.h"
#include <wrl.h>
#include <vector>

using Microsoft::WRL::ComPtr;

struct SimpleVertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};

class TriangleObject : public GameObject {
public:
    TriangleObject() = default;
    virtual ~TriangleObject() = default;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

private:
    void CreateGeometry(Renderer* renderer);

    // D3D12 resources
    ComPtr<ID3D12Resource>      vertexBuffer;
    ComPtr<ID3D12Resource>      indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW    vbView = {};
    D3D12_INDEX_BUFFER_VIEW     ibView = {};

    std::vector<SimpleVertex> vertices;
    std::vector<UINT>         indices;
};
