#pragma once

#include "GameObject.h"
#include <vector>
#include <string>
#include <DirectXMath.h>

using namespace DirectX;

class HeightMapTerrain : public GameObject
{
public:
    HeightMapTerrain(const std::wstring& heightMapPath, int width, int height, float scale, float vertexDist = 7.0f);
    virtual ~HeightMapTerrain() = default;

    virtual bool Initialize(Renderer* renderer) override;
    virtual void Update(float deltaTime) override;
    virtual void Render(Renderer* renderer) override;

    void SetVertexDistance(float vertexDist);

private:

    bool CreateRasterizerState(ID3D11Device* device);

    void LoadHeightMap();
    void CreateMeshFromHeightMap();
    XMFLOAT3 CalculateNormal(int x, int z);  // 법선 벡터 계산

private:
    std::wstring heightMapPath;
    int mapWidth, mapHeight;
    float heightScale;
    float vertexDistance;

    std::vector<uint8_t> heightData;

    std::vector<MeshVertex> vertices;
    std::vector<UINT> indices;

    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
};
