#pragma once

#include "GameObject.h"
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;

struct SimpleVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

class Renderer;

class TriangleObject : public GameObject
{
public:
    TriangleObject() = default;
    ~TriangleObject() override = default;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime, Renderer* renderer, UINT objectIndex) override;
    void Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex) override;

private:
    void CreateGeometry(Renderer* renderer);

    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    D3D12_INDEX_BUFFER_VIEW  indexBufferView = {};

    std::vector<SimpleVertex> vertices;
    std::vector<UINT>         indices;
};