#pragma once

#include <d3d11.h>
#include <vector>
#include <DirectXMath.h>
#include <memory>
#include <wrl/client.h>

using namespace DirectX;
using namespace Microsoft::WRL;

struct MeshVertex {
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 texCoords;
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    bool Initialize(ID3D11Device* device, const std::vector<MeshVertex>& vertices, const std::vector<unsigned int>& indices);

    ID3D11Buffer* GetVertexBuffer() const;
    ID3D11Buffer* GetIndexBuffer() const;
    unsigned int GetIndexCount() const;

    static std::shared_ptr<Mesh> CreateCube(ID3D11Device* device);
    static std::shared_ptr<Mesh> CreateQuad(ID3D11Device* device);

private:
    bool CreateBuffers(ID3D11Device* device, const std::vector<MeshVertex>& vertices, const std::vector<unsigned int>& indices);

    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    unsigned int indexCount;
};
