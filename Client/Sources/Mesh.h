#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <stdexcept>

class Renderer;

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct MeshVertex {
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 texCoord;
    XMFLOAT3 tangent;
};


class Mesh {
public:
    Mesh() = default;
    ~Mesh() = default;

    /**
     * Uploads vertices/indices into DEFAULT - heap buffers via the renderer¡¯s
     * dedicated COPY queue & waits for completion. Returns false on failure.
     */
    bool Initialize(Renderer* renderer,
        const std::vector<MeshVertex>& vertices,
        const std::vector<uint32_t>& indices);

    // GPU views
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexView; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView()  const { return indexView; }
    uint32_t                        GetIndexCount() const { return indexCount; }

    static std::shared_ptr<Mesh> CreateCube(Renderer* renderer);
    static std::shared_ptr<Mesh> CreateQuad(Renderer* renderer);

private:
    bool UploadBuffers(Renderer* renderer,
        const void* vertexData, size_t vertexByteSize,
        const void* indexData, size_t indexByteSize);

    ComPtr<ID3D12Resource> vertexBuffer; // DEFAULT
    ComPtr<ID3D12Resource> indexBuffer;  // DEFAULT

    D3D12_VERTEX_BUFFER_VIEW vertexView{};
    D3D12_INDEX_BUFFER_VIEW  indexView{};
    uint32_t indexCount = 0;
};