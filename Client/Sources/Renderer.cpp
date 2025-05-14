#include "Renderer.h"
#include "DebugManager.h"
#include "DebugRenderer.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <dxgi.h>
#include <d3d11.h>
#include <wrl.h>
#include <sstream>


using namespace Microsoft::WRL;

Renderer::Renderer() {
}

Renderer::~Renderer() {
    Cleanup();
}

bool Renderer::Init(HWND hwnd, int width, int height) {

    if (!InitD3D(hwnd, width, height))
        return false;

    shaderManager = std::make_shared<ShaderManager>();

    if (!shaderManager->Init(device.Get())) {
        MessageBox(nullptr, L"ShaderManager initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!shaderManager->CompileShaders()) {
        MessageBox(nullptr, L"Shader compilation failed", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    mainCamera = std::make_unique<Camera>();
    mainCamera->SetPosition(XMFLOAT3(0.0f, 0.0f, -5.0f));

    mainCamera->SetRotationQuat(XMQuaternionIdentity());
    mainCamera->SetPerspective(XM_PIDIV4, static_cast<float>(width) / height, 0.1f, 100.0f);
    mainCamera->UpdateViewMatrix();

    if (!InitGlobalBuffer()) {
        MessageBox(nullptr, L"Failed to create global constant buffer", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // 라이팅 버퍼 생성
    if (!SetupLightingConstantBuffer()) {
        MessageBox(nullptr, L"Failed to create lighting buffer", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // 필요에 따라선 TextureSampler는 여러 개 둘 수도 있음.
    if (!SetupTextureSampler()) {
        MessageBox(nullptr, L"Failed to create lighting buffer", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    DebugRenderer::GetInstance().Initialize(this);

    return true;
}

void Renderer::Cleanup() {
    if (deviceContext) {
        deviceContext->ClearState();
    }
}

ID3D11Device* Renderer::GetDevice() const
{
    return device.Get();
}

ID3D11DeviceContext* Renderer::GetDeviceContext() const
{
    return deviceContext.Get();
}

ShaderManager* Renderer::GetShaderManager()
{
    return shaderManager.get();
}

void Renderer::AddGameObject(std::shared_ptr<GameObject> obj)
{
    gameObjects.push_back(obj);
}

void Renderer::RemoveGameObject(std::shared_ptr<GameObject> obj)
{
    gameObjects.erase(
        std::remove(gameObjects.begin(), gameObjects.end(), obj),
        gameObjects.end()
    );
}

bool Renderer::InitD3D(HWND hwnd, int width, int height) {
    // DXGI_SWAP_CHAIN_DESC 설정
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scd.Windowed = TRUE;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;


    // D3D11 장치, 컨텍스트, 스왑 체인 생성
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                    // 기본 어댑터
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,                           // 플래그
        nullptr, 0,                  // default feature level
        D3D11_SDK_VERSION,
        &scd,
        &swapChain,
        &device,
        nullptr,
        &deviceContext
    );

    if (FAILED(hr)) {
        // 하드웨어 드라이버 실패 시, WARP로 재시도
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            nullptr, 0,
            D3D11_SDK_VERSION,
            &scd,
            &swapChain,
            &device,
            nullptr,
            &deviceContext
        );

        if (FAILED(hr)) {
            MessageBox(nullptr, L"Failed to create D3D11 device and swap chain.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    // ID3D11Texture2D 는 단순히 메모리
    // Render Target View 는 위의 메모리를 렌더링 용도로 사용하는 뷰 객체
    // Depth Stencil View 는 위의 메모리를 깊이-스텐실 용도로 사용하는 뷰 객체


    // 스왑 체인으로부터 백버퍼 얻어오기
    ComPtr<ID3D11Texture2D> backBuffer;
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to get back buffer from swap chain.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // 백 버퍼 텍스처로부터 렌더 타겟 뷰(Render Target View) 생성
    hr = device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView);
    if (FAILED(hr)) {
        std::wstringstream wss;
        wss << L"CreateRenderTargetView failed. HRESULT = 0x"
            << std::hex << std::uppercase << hr;
        MessageBox(nullptr, wss.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Depth Stencil Buffer 생성
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;

    ComPtr<ID3D11Texture2D> depthStencilBuffer;
    hr = device->CreateTexture2D(&depthDesc, nullptr, &depthStencilBuffer);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create depth stencil buffer.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // 생성한 Depth Stencil 버퍼로부터 Render Target View 생성 
    hr = device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, &depthStencilView);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create depth stencil view.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // DepthStencilState 생성
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = FALSE;

    device->CreateDepthStencilState(&dsDesc, &depthStencilState);


    // 뷰포트 설정
    viewport.Width = static_cast<FLOAT>(width);
    viewport.Height = static_cast<FLOAT>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;

    deviceContext->RSSetViewports(1, &viewport);


    // 레스터 라이저 생성
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;            // 일반 렌더링 (와이어프레임은 D3D11_FILL_WIREFRAME)
    rasterDesc.CullMode = D3D11_CULL_BACK;              // 보통 BACK 페이스 컬링
    rasterDesc.FrontCounterClockwise = FALSE;           // 시계 방향을 전면으로 봄
    rasterDesc.DepthClipEnable = TRUE;                  // Z 클리핑 활성화

    ComPtr<ID3D11RasterizerState> rasterizerState;
    hr = device->CreateRasterizerState(&rasterDesc, &rasterizerState);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create rasterizer state.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}


void Renderer::Update(float deltaTime)
{   
    for (auto& gameObject : gameObjects) {
        gameObject->Update(deltaTime);
    }
}

void Renderer::Render() {

    // 렌더 타겟 뷰 설정
    deviceContext->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());

    // 화면을 검은색으로 초기화
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    deviceContext->ClearRenderTargetView(renderTargetView.Get(), clearColor);
    deviceContext->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);


    // 라이팅 정보 업데이트
    lightingData.cameraWorld = mainCamera->GetPosition();
    std::copy(lights.begin(), lights.end(), lightingData.lights);

    deviceContext->UpdateSubresource(
        lightingConstantBuffer.Get(), 0, nullptr, &lightingData, 0, 0);


    deviceContext->OMSetDepthStencilState(depthStencilState.Get(), 0);

    for (auto& obj : gameObjects) {
        obj->Render(this);
    }

    // 2D UI 렌더링 (UIManager 사용)



    DebugRenderer::GetInstance().Render(this);

    DebugManager::GetInstance().RenderMessages();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    swapChain->Present(0, 0);
}

bool Renderer::InitGlobalBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = sizeof(CB_Global);
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    CB_Global initial = {};
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &initial;

    HRESULT hr = device->CreateBuffer(&desc, &initData, cbGlobalBuffer.GetAddressOf());
    return SUCCEEDED(hr);
}

void Renderer::UpdateGlobalTime(float totalTime)
{
    CB_Global globalCB = {};
    globalCB.time = totalTime;

    deviceContext->UpdateSubresource(cbGlobalBuffer.Get(), 0, nullptr, &globalCB, 0, 0);

    deviceContext->VSSetConstantBuffers(3, 1, cbGlobalBuffer.GetAddressOf());
    deviceContext->PSSetConstantBuffers(3, 1, cbGlobalBuffer.GetAddressOf());
}

Camera* Renderer::GetCamera()
{
    return mainCamera.get();
}

void Renderer::SetCamera(std::shared_ptr<Camera> newCamera)
{
    mainCamera = newCamera;
}

ID3D11Buffer* Renderer::GetLightingConstantBuffer()
{
    return lightingConstantBuffer.Get();
}

ID3D11SamplerState* Renderer::GetSamplerState() const
{
    return samplerState.Get();
}

bool Renderer::SetupLightingConstantBuffer()
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(CB_Lighting);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    HRESULT hr = device->CreateBuffer(&cbDesc, nullptr, lightingConstantBuffer.GetAddressOf());
    return SUCCEEDED(hr);
}

bool Renderer::SetupTextureSampler()
{
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = device->CreateSamplerState(&sampDesc, &samplerState);
    return SUCCEEDED(hr);
}

void Renderer::SetupDefaultLights()
{
    lights.clear();

    Light dirLight = {};
    dirLight.type = static_cast<int>(LightType::Directional);
    dirLight.direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
    dirLight.strength = XMFLOAT3(0.8f, 0.8f, 0.8f);
    dirLight.color = XMFLOAT3(1.0f, 1.0f, 1.0f);
    dirLight.specPower = 32.0f;

    Light pointLight = {};
    pointLight.type = static_cast<int>(LightType::Point);
    pointLight.position = XMFLOAT3(0.0f, 2.0f, -2.0f);
    pointLight.strength = XMFLOAT3(0.6f, 0.6f, 0.6f);
    pointLight.color = XMFLOAT3(1.0f, 1.0f, 1.0f);
    pointLight.falloffStart = 1.0f;
    pointLight.falloffEnd = 6.0f;
    pointLight.specPower = 32.0f;

    Light spotLight = {};
    spotLight.type = static_cast<int>(LightType::Spot);
    spotLight.position = XMFLOAT3(0.0f, 4.0f, -3.0f);
    spotLight.direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
    spotLight.strength = XMFLOAT3(1.0f, 1.0f, 1.0f);
    spotLight.color = XMFLOAT3(1.0f, 1.0f, 1.0f);
    spotLight.falloffStart = 2.0f;
    spotLight.falloffEnd = 10.0f;
    spotLight.specPower = 64.0f;

    lights.push_back(dirLight);
    lights.push_back(pointLight);
    lights.push_back(spotLight);
}

int Renderer::GetViewportWidth() const
{
    return static_cast<int>(viewport.Width);
}

int Renderer::GetViewportHeight() const
{
    return static_cast<int>(viewport.Height);
}
