#pragma once

#include "GameObject.h"
#include <wrl.h>
#include <vector>

using namespace DirectX;
using namespace Microsoft::WRL;

struct SimpleVertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

class TriangleObject : public GameObject
{
public:
    TriangleObject();
    virtual ~TriangleObject() = default;

    bool Initialize(Renderer* renderer);
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

private:
    void CreateGeometry();

    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;

    std::vector<SimpleVertex> vertices;
    std::vector<UINT> indices;

};
