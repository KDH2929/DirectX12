#include "HeightMapTerrain.h"
#include "Renderer.h"
#include "DebugManager.h"

#include <fstream>

HeightMapTerrain::HeightMapTerrain(const std::wstring& path, int width, int height, float scale, float vertexDist)
    : heightMapPath(path), mapWidth(width), mapHeight(height), heightScale(scale), vertexDistance(vertexDist)
{}

bool HeightMapTerrain::Initialize(Renderer* renderer)
{
    GameObject::Initialize(renderer);

    LoadHeightMap();
    CreateMeshFromHeightMap();

    auto device = renderer->GetDevice();

    // Vertex Buffer
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = UINT(sizeof(MeshVertex) * vertices.size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vinitData = {};
    vinitData.pSysMem = vertices.data();

    device->CreateBuffer(&vbd, &vinitData, vertexBuffer.GetAddressOf());

    // Index Buffer
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = UINT(sizeof(UINT) * indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = indices.data();

    device->CreateBuffer(&ibd, &iinitData, indexBuffer.GetAddressOf());

    // Shader 설정
    auto shaderManager = renderer->GetShaderManager();
    shaderInfo.vertexShaderName = L"PhongVertexShader";
    shaderInfo.pixelShaderName = L"PhongPixelShader";

    vertexShader = shaderManager->GetVertexShader(shaderInfo.vertexShaderName);
    pixelShader = shaderManager->GetPixelShader(shaderInfo.pixelShaderName);
    inputLayout = shaderManager->GetInputLayout(shaderInfo.vertexShaderName);

    // Rasterizer 설정
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.CullMode = shaderInfo.cullMode;
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = true;

    device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf());

    // Material 기본값
    materialData.ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
    materialData.diffuse = XMFLOAT3(0.6f, 0.6f, 0.6f);
    materialData.specular = XMFLOAT3(0.1f, 0.1f, 0.1f);
    materialData.useTexture = 0;


    // 콜라이더 크기 설정
    PxVec3 halfExtents = { 30.0f, 5.0f, 30.0f };  // 초기값

    // 지형 메시의 폭과 높이를 고려하여 콜라이더 크기 조정
    float terrainWidth = static_cast<float>(mapWidth) * vertexDistance;
    float terrainHeight = static_cast<float>(mapHeight) * vertexDistance;

    halfExtents.x = terrainWidth / 2.0f;
    halfExtents.z = terrainHeight / 2.0f;

    XMFLOAT3 terrainPos = GetPosition();
    PxVec3 colliderCenter = PxVec3(
        terrainPos.x + halfExtents.x,
        terrainPos.y - halfExtents.y,
        terrainPos.z + halfExtents.z
    );

    // 콜라이더 생성
    PxRigidStatic* actor = PhysicsManager::GetInstance()->CreateStaticBox(
        colliderCenter,
        halfExtents,
        CollisionLayer::Default
    );

    BindPhysicsActor(actor);


    return true;
}

void HeightMapTerrain::Update(float deltaTime)
{
    UpdateWorldMatrix();
}

void HeightMapTerrain::Render(Renderer* renderer)
{
    DrawPhysicsColliderDebug();

    auto context = renderer->GetDeviceContext();

    // 셰이더 & 입력 레이아웃
    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);
    context->RSSetState(rasterizerState.Get());

    // 정점/인덱스 버퍼 설정
    UINT stride = sizeof(MeshVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // CB_MVP (b0) 구성
    CB_MVP mvpData;
    XMMATRIX view = renderer->GetCamera()->GetViewMatrix();
    XMMATRIX proj = renderer->GetCamera()->GetProjectionMatrix();

    mvpData.model = XMMatrixTranspose(worldMatrix);
    mvpData.view = XMMatrixTranspose(view);
    mvpData.projection = XMMatrixTranspose(proj);
    mvpData.modelInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));

    context->UpdateSubresource(constantMVPBuffer.Get(), 0, nullptr, &mvpData, 0, 0);
    context->VSSetConstantBuffers(0, 1, constantMVPBuffer.GetAddressOf());  // b0

    // 조명 (b1)
    ID3D11Buffer* lightCB = renderer->GetLightingConstantBuffer();
    context->PSSetConstantBuffers(1, 1, &lightCB);

    // 머티리얼 (b2)
    context->UpdateSubresource(constantMaterialBuffer.Get(), 0, nullptr, &materialData, 0, 0);
    context->PSSetConstantBuffers(2, 1, constantMaterialBuffer.GetAddressOf());

    // 드로우
    context->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
}

void HeightMapTerrain::SetVertexDistance(float vertexDist)
{
    vertexDistance = vertexDist;
}

void HeightMapTerrain::LoadHeightMap()
{
    FILE* file = nullptr;
    _wfopen_s(&file, heightMapPath.c_str(), L"rb");

    if (!file) {
        return;
    }

    heightData.resize(mapWidth * mapHeight);
    fread(heightData.data(), 1, heightData.size(), file);
    fclose(file);

    if (heightData.empty()) {
        throw std::runtime_error("Failed to load heightmap data.");
    }
}

void HeightMapTerrain::CreateMeshFromHeightMap()
{
    vertices.clear();
    indices.clear();

    // vertices 생성
    for (int z = 0; z < mapHeight; ++z)
    {
        for (int x = 0; x < mapWidth; ++x)
        {
            if (!heightData.empty()) {
                // 높이 계산
                float height = heightData[z * mapWidth + x] / 255.0f * heightScale;

                float posX = (float)x * vertexDistance;
                float posZ = (float)z * vertexDistance;
                XMFLOAT3 pos = XMFLOAT3(posX, height, posZ);

                // 노멀 계산
                XMFLOAT3 normal = CalculateNormal(x, z);

                // UV 계산
                XMFLOAT2 uv = XMFLOAT2((float)x / mapWidth, (float)z / mapHeight);
                vertices.push_back({ pos, normal, uv });
            }
        }
    }

    // indices 생성
    for (int z = 0; z < mapHeight - 1; ++z)
    {
        for (int x = 0; x < mapWidth - 1; ++x)
        {
            int start = z * mapWidth + x;

            // 인덱스 범위 계산
            if (start + mapWidth + 1 < vertices.size()) {
                indices.push_back(start);
                indices.push_back(start + mapWidth);
                indices.push_back(start + 1);

                indices.push_back(start + 1);
                indices.push_back(start + mapWidth);
                indices.push_back(start + mapWidth + 1);
            }
        }
    }
}

// 주변 정점들로부터 법선 벡터 계산
XMFLOAT3 HeightMapTerrain::CalculateNormal(int x, int z)
{
    // 경계 검사를 통한 인덱스 범위 초과 방지
    float heightL = (x > 0) ? heightData[z * mapWidth + (x - 1)] : heightData[z * mapWidth + x]; // 왼쪽
    float heightR = (x < mapWidth - 1) ? heightData[z * mapWidth + (x + 1)] : heightData[z * mapWidth + x]; // 오른쪽
    float heightD = (z > 0) ? heightData[(z - 1) * mapWidth + x] : heightData[z * mapWidth + x]; // 아래
    float heightU = (z < mapHeight - 1) ? heightData[(z + 1) * mapWidth + x] : heightData[z * mapWidth + x]; // 위

    // 경사 계산 (이웃과의 높이 차이)
    float dx = (heightR - heightL) * heightScale;
    float dz = (heightU - heightD) * heightScale;

    // 법선 벡터 계산
    XMFLOAT3 normal = { dx, 1.0f, dz };
    XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&normal)));  // 벡터 정규화

    return normal;
}
