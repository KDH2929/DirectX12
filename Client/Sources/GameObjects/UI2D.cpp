#include "UI2D.h"
#include "Renderer.h"

UI2D::UI2D(XMFLOAT2 position, XMFLOAT2 size, std::shared_ptr<Texture> texture, XMFLOAT4 color)
    : position(position), size(size), texture(texture), color(color) {}

bool UI2D::Initialize(Renderer* renderer)
{
    GameObject::Initialize(renderer);

    auto shaderManager = renderer->GetShaderManager();

    shaderInfo.vertexShaderName = L"UI2DVertexShader";
    shaderInfo.pixelShaderName = L"UI2DPixelShader";

    vertexShader = shaderManager->GetVertexShader(shaderInfo.vertexShaderName);
    pixelShader = shaderManager->GetPixelShader(shaderInfo.pixelShaderName);

    materialData.diffuse = XMFLOAT3(color.x, color.y, color.z);  // UI 색상 전달
    materialData.alpha = color.w;
    materialData.useTexture = (texture != nullptr) ? 1 : 0;


    quadMesh = Mesh::CreateQuad(renderer->GetDevice());

    if (!quadMesh) {
        return false;  // 메시 생성 실패
    }

    CreateDepthStencilState(renderer->GetDevice());
    CreateRasterizerState(renderer->GetDevice());

    return true;
}

void UI2D::Update(float deltaTime) {
    // UI 업데이트 (애니메이션, 마우스 오버 등)
}

void UI2D::Render(Renderer* renderer) {
    if (!texture || !quadMesh) return;  // 텍스처와 메시가 없으면 렌더링하지 않음


    auto context = renderer->GetDeviceContext();
    
    // DepthStencilState 설정
    ID3D11DepthStencilState* prevDepthStencilState = nullptr;
    context->OMGetDepthStencilState(&prevDepthStencilState, nullptr);
    context->OMSetDepthStencilState(depthStencilState.Get(), 0);

    ID3D11RasterizerState* prevRasterizerState = nullptr;
    context->RSGetState(&prevRasterizerState);
    context->RSSetState(rasterizerState.Get());


    XMMATRIX view = XMMatrixIdentity(); // UI는 카메라 이동/회전 없음
    XMMATRIX proj = XMMatrixOrthographicOffCenterLH(
        0.0f, static_cast<float>(renderer->GetViewportWidth()),
        static_cast<float>(renderer->GetViewportHeight()), 0.0f,
        0.0f, 1.0f
    );

    // UI 요소는 보통 화면에 정렬되기 때문에 billboarding은 적용하지 않음
    // 단순히 UI 요소를 그리기 위한 world 변환 행렬을 계산

    // UI의 위치와 크기를 고려한 월드 변환 행렬
    XMMATRIX world = XMMatrixScaling(size.x, size.y, 1.0f) *
        XMMatrixTranslation(position.x, position.y, 0.0f);

    // MVP 상수 버퍼 설정
    CB_MVP mvpData;
    mvpData.model = XMMatrixTranspose(world);  // 월드 변환 행렬
    mvpData.view = XMMatrixTranspose(view);    // 카메라의 뷰 매트릭스
    mvpData.projection = XMMatrixTranspose(proj);  // 카메라의 투영 매트릭스
    mvpData.modelInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, world));  // 모델의 역행렬

    context->UpdateSubresource(constantMVPBuffer.Get(), 0, nullptr, &mvpData, 0, 0);
    context->VSSetConstantBuffers(0, 1, constantMVPBuffer.GetAddressOf());  // 상수 버퍼 전달

    // 머티리얼 상수 버퍼 업데이트
    context->UpdateSubresource(constantMaterialBuffer.Get(), 0, nullptr, &materialData, 0, 0);
    context->PSSetConstantBuffers(2, 1, constantMaterialBuffer.GetAddressOf());  // 머티리얼 정보 전달

    // 셰이더 설정
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);

    // 텍스처 바인딩
    if (texture) {
        ID3D11ShaderResourceView* srv = texture->GetShaderResourceView();
        if (srv) {
            ID3D11SamplerState* sampler = renderer->GetSamplerState();
            context->PSSetShaderResources(0, 1, &srv);  // 셰이더에 텍스처 전달
            context->PSSetSamplers(0, 1, &sampler);  // 셰이더에 샘플러 전달
        }
    }

    // 정점(Vertex) 및 인덱스(Index) 버퍼 바인딩
    UINT stride = sizeof(MeshVertex);  // 정점의 크기
    UINT offset = 0;  // 오프셋 (첫 번째 정점부터 시작)

    // 메시의 정점 및 인덱스 버퍼 바인딩
    ID3D11Buffer* vb = quadMesh->GetVertexBuffer();
    ID3D11Buffer* ib = quadMesh->GetIndexBuffer();

    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // UI2D 사각형 그리기
    context->DrawIndexed(quadMesh->GetIndexCount(), 0, 0);  // 인덱스 6개 (두 삼각형으로 구성)

    // 이전 DepthStencilState 복원
    context->OMSetDepthStencilState(prevDepthStencilState, 0);
    if (prevDepthStencilState) prevDepthStencilState->Release();

    context->RSSetState(prevRasterizerState);
    if (prevRasterizerState) prevRasterizerState->Release();
}

void UI2D::SetPosition(float x, float y) {
    position.x = x;
    position.y = y;
}

XMFLOAT2 UI2D::GetPosition() const {
    return position;
}

void UI2D::SetSize(float width, float height) {
    size.x = width;
    size.y = height;
}

XMFLOAT2 UI2D::GetSize() const {
    return size;
}

void UI2D::SetColor(XMFLOAT4 newColor) {
    color = newColor;
}

XMFLOAT4 UI2D::GetColor() const {
    return color;
}

bool UI2D::CreateDepthStencilState(ID3D11Device* device)
{
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = FALSE;              // 깊이 테스트 비활성화
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_ALWAYS; // 깊이 비교 함수 설정 (항상 true)

    desc.StencilEnable = FALSE; // 스텐실 비활성화

    return SUCCEEDED(device->CreateDepthStencilState(&desc, depthStencilState.GetAddressOf()));
}

bool UI2D::CreateRasterizerState(ID3D11Device* device)
{
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;  // Culling 비활성화
    rsDesc.FrontCounterClockwise = TRUE; // 반시계방향을 앞면으로 설정 (필요에 따라 설정)
    rsDesc.DepthClipEnable = TRUE;

    return SUCCEEDED(device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf()));
}
