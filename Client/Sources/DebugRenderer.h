#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include "physx/PxPhysicsAPI.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using namespace physx;

class Renderer;  // Forward declaration

class DebugRenderer {
public:
    static DebugRenderer& GetInstance();

    bool Initialize(Renderer* renderer);
    void DrawBox(const PxVec3& center, const PxVec3& halfExtents, const PxQuat& rotation);
    void DrawSphere(const PxVec3& center, float radius);
    void Render(Renderer* renderer);

private:
    DebugRenderer() = default;

    struct LineVertex {
        XMFLOAT3 position;
    };

    struct CB_MVP {
        XMMATRIX model;
        XMMATRIX view;
        XMMATRIX projection;
    };

    enum class DebugShapeType { Box, Sphere };

    struct DrawCall {
        XMMATRIX world;
        DebugShapeType shapeType;
    };

    void CreateBoxGeometry();
    void CreateSphereGeometry();

    std::vector<LineVertex> boxVertices;
    std::vector<UINT> boxIndices;
    std::vector<LineVertex> sphereVertices;
    std::vector<UINT> sphereIndices;

    ComPtr<ID3D11Buffer> boxVertexBuffer;
    ComPtr<ID3D11Buffer> boxIndexBuffer;
    ComPtr<ID3D11Buffer> sphereVB;
    ComPtr<ID3D11Buffer> sphereIB;
    ComPtr<ID3D11Buffer> constantBuffer;

    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11RasterizerState> wireframeState;

    std::vector<DrawCall> drawCalls;
    Renderer* renderer = nullptr;
};
