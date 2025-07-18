#include "Renderer.h"
#include "RenderPass/ForwardOpaquePass.h"
#include "RenderPass/ForwardTransparentPass.h"
#include "RenderPass/PostProcessPass.h"
#include <stdexcept>

Renderer::Renderer() {}

Renderer::~Renderer() {
    Cleanup();
}

bool Renderer::Initialize(HWND hwnd, int width, int height) {
    if (!InitD3D(hwnd, width, height))
        return false;

    // Managers

    rootSignatureManager = std::make_unique<RootSignatureManager>(device.Get());
    assert(rootSignatureManager && "rootSignatureManager nullptr!");
    if (!rootSignatureManager->InitializeDescs())
        return false;

    shaderManager = std::make_unique<ShaderManager>(device.Get());
    std::vector<ShaderCompileDesc> shaderDescs = {
        { L"TriangleVertexShader", L"Shaders/TriangleVertexShader.hlsl", "VSMain", "vs_5_0" },
        { L"TrianglePixelShader",  L"Shaders/TrianglePixelShader.hlsl",  "PSMain", "ps_5_0" },
        { L"PhongVertexShader",    L"Shaders/PhongVertexShader.hlsl",    "VSMain", "vs_5_1" },
        { L"PhongPixelShader",     L"Shaders/PhongPixelShader.hlsl",     "PSMain", "ps_5_1" },
        { L"PbrVertexShader",      L"Shaders/PbrVertexShader.hlsl",      "VSMain", "vs_5_1" },
        { L"PbrPixelShader",       L"Shaders/PbrPixelShader.hlsl",       "PSMain", "ps_5_1" },

        { L"SkyboxVertexShader",     L"Shaders/SkyboxVertexShader.hlsl",     "VSMain", "vs_5_0" },
        { L"SkyboxPixelShader",      L"Shaders/SkyboxPixelShader.hlsl",      "PSMain", "ps_5_0" },
        
        {L"DebugNormalVS", L"Shaders/DebugNormalShader.hlsl", "VSMain", "vs_5_0"},
        {L"DebugNormalGS", L"Shaders/DebugNormalShader.hlsl", "GSMain", "gs_5_0"},
        {L"DebugNormalPS", L"Shaders/DebugNormalShader.hlsl", "PSMain", "ps_5_0"},

        { L"OutlinePostEffectVS",   L"Shaders/OutlinePostEffect.hlsl",  "VSMain", "vs_5_0" },
        { L"OutlinePostEffectPS",   L"Shaders/OutlinePostEffect.hlsl",  "PSMain", "ps_5_0" },
    };

    if (!shaderManager->CompileAll(shaderDescs))
        return false;

    psoManager = std::make_unique<PipelineStateManager>(this);
    if (!psoManager->InitializePSOs())
        return false;

    {
        auto pso = psoManager->Get(L"PhongPSO");
        assert(pso && "PhongPSO not found! Did InitializePSOs() register it?");
        if (!pso)
            throw std::runtime_error("Failed to find PhongPSO after initialization");
    }


    textureManager = std::make_unique<TextureManager>();
    if (!textureManager->Initialize(this, descriptorHeapManager.get()))
        return false;

    // Setup camera
    mainCamera = std::make_shared<Camera>();
    mainCamera->SetPosition({ 0, 0, -30 });
    mainCamera->SetPerspective(XM_PIDIV4, float(width) / height, 0.1f, 1000.f);

    // Global constant buffer (b3)
    {
        CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(256);
        ThrowIfFailed(device->CreateCommittedResource(
            &props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&globalConstantBuffer)));
        globalConstantBuffer->Map(0, nullptr,
            reinterpret_cast<void**>(&mappedGlobalPtr));
    }

    // Lighting constant buffer (b1)
    {
        CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(256);
        ThrowIfFailed(device->CreateCommittedResource(
            &props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&lightingConstantBuffer)));
        lightingConstantBuffer->Map(0, nullptr,
            reinterpret_cast<void**>(&mappedLightingPtr));

        // 기본 라이팅 값 설정
        lightingData = {};
        lightingData.cameraWorld = mainCamera->GetPosition();

        // 카메라 기준 벡터
        auto cameraPos = mainCamera->GetPosition();
        auto cameraForward = mainCamera->GetForwardVector();
        auto cameraUp = mainCamera->GetUpVector();

        // Directional
        auto& Directional = lightingData.lights[0];
        Directional.type = 0;
        Directional.color = { 1,1,1 };
        Directional.strength = { 1,1,1 };
        Directional.direction = { 0,-1,0 };
        Directional.falloffStart = 0;
        Directional.falloffEnd = 0;      // 사용 안 함
        Directional.specPower = 32;

        // Point
        auto& Point = lightingData.lights[1];
        Point.type = 1;
        Point.color = { 1,1,1 };
        Point.strength = { 2,2,2 };
        Point.position = { cameraPos.x, cameraPos.y + 2.0f, cameraPos.z };
        Point.falloffStart = 1;
        Point.falloffEnd = 20;
        Point.specPower = 32;

        // Spot
        auto& Spot = lightingData.lights[2];
        Spot.type = 2;
        Spot.color = { 1,1,1 };
        Spot.strength = { 2,2,2 };
        Spot.position = { cameraPos.x + cameraForward.x * 3.0f,
                              cameraPos.y + cameraForward.y * 3.0f,
                              cameraPos.z + cameraForward.z * 3.0f };
        Spot.direction = { cameraForward.x, cameraForward.y, cameraForward.z };
        Spot.falloffStart = 5;
        Spot.falloffEnd = 30;
        Spot.specPower = 64;

        memcpy(mappedLightingPtr, &lightingData, sizeof(lightingData));
    }

    descriptorHeapManager->CreateWrapSampler(device.Get());
    descriptorHeapManager->CreateClampSampler(device.Get());


    // 렌더패스 초기화
    renderPasses[static_cast<size_t>(PassIndex::ForwardOpaque)] = std::make_unique<ForwardOpaquePass>();
    renderPasses[static_cast<size_t>(PassIndex::ForwardTransparent)] = std::make_unique<ForwardTransparentPass>();
    renderPasses[static_cast<size_t>(PassIndex::PostProcess)] = std::make_unique<PostProcessPass>();

    for (size_t i = 0; i < static_cast<size_t>(PassIndex::Count); ++i)
    {
        renderPasses[i]->Initialize(this);
    }


    return true;
}

void Renderer::Cleanup() {
    WaitForDirectQueue();
    if (directFenceEvent != nullptr && directFenceEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(directFenceEvent);
        directFenceEvent = nullptr;
    }
}

void Renderer::AddGameObject(std::shared_ptr<GameObject> object) {

    gameObjects.push_back(object);

    if (object->IsTransparent())
    {
        transparentObjects.push_back(object);
    }
    else
    {
        opaqueObjects.push_back(object);
    }
}

void Renderer::RemoveGameObject(std::shared_ptr<GameObject> object) {

    gameObjects.erase(
        std::remove(gameObjects.begin(), gameObjects.end(), object),
        gameObjects.end());

    // 투명/불투명 리스트에서도 제거
    auto removeFrom = [&](auto& list) {
        list.erase(
            std::remove(list.begin(), list.end(), object),
            list.end());
        };

    if (object->IsTransparent()) {
        removeFrom(transparentObjects);
    }

    else {
        removeFrom(opaqueObjects);
    }
}

const std::vector<std::shared_ptr<GameObject>>& Renderer::GetOpaqueObjects() const
{
    return opaqueObjects;
}

const std::vector<std::shared_ptr<GameObject>>& Renderer::GetTransparentObjects() const
{
    return transparentObjects;
}

const std::vector<std::shared_ptr<GameObject>>& Renderer::GetAllGameObjects() const
{
    return gameObjects;
}

void Renderer::Update(float deltaTime) {

    for (auto& object : gameObjects) {
        object->Update(deltaTime);
    }

    for (auto& pass : renderPasses) {
        pass->Update(deltaTime);
    }

    ImGui::Begin("Lighting Controls");

    // Directional Light
    {
        Light& L = lightingData.lights[0];
        ImGui::Text("Directional Light");
        ImGui::ColorEdit3("Color##Dir", reinterpret_cast<float*>(&L.color));
        ImGui::SliderFloat("Strength##Dir", &L.strength.x, 0.0f, 10.0f);
        L.strength.y = L.strength.z = L.strength.x;
        ImGui::DragFloat3("Direction##Dir",
            reinterpret_cast<float*>(&L.direction),
            0.01f,
            -5.0f, 5.0f);  // ← 범위 확장
    }
    ImGui::Separator();

    // Point Light
    {
        Light& L = lightingData.lights[1];
        ImGui::Text("Point Light");
        ImGui::ColorEdit3("Color##Point", reinterpret_cast<float*>(&L.color));
        ImGui::SliderFloat("Strength##Point", &L.strength.x, 0.0f, 10.0f);
        L.strength.y = L.strength.z = L.strength.x;
        ImGui::DragFloat3("Position##Point",
            reinterpret_cast<float*>(&L.position),
            0.1f,
            -5.0f, 5.0f);
        ImGui::SliderFloat("FalloffStart##Point", &L.falloffStart, 0.0f, 20.0f);
        ImGui::SliderFloat("FalloffEnd##Point", &L.falloffEnd, 1.0f, 100.0f);
    }
    ImGui::Separator();

    // Spot Light
    {
        Light& L = lightingData.lights[2];
        ImGui::Text("Spot Light");
        ImGui::ColorEdit3("Color##Spot", reinterpret_cast<float*>(&L.color));
        ImGui::SliderFloat("Strength##Spot", &L.strength.x, 0.0f, 10.0f);
        L.strength.y = L.strength.z = L.strength.x;
        ImGui::DragFloat3("Position##Spot",
            reinterpret_cast<float*>(&L.position),
            0.1f,
            -5.0f, 5.0f);
        ImGui::DragFloat3("Direction##Spot",
            reinterpret_cast<float*>(&L.direction),
            0.01f,
            -5.0f, 5.0f);
        ImGui::SliderFloat("FalloffStart##Spot", &L.falloffStart, 0.0f, 20.0f);
        ImGui::SliderFloat("FalloffEnd##Spot", &L.falloffEnd, 1.0f, 100.0f);
        ImGui::SliderFloat("SpecPower##Spot", &L.specPower, 1.0f, 256.0f);
    }

    ImGui::End();

    lightingData.cameraWorld = mainCamera->GetPosition();
    memcpy(mappedLightingPtr, &lightingData, sizeof(lightingData));
}

void Renderer::Render() {
    PopulateCommandList();

    ID3D12CommandList* lists[] = { directCommandList.Get() };
    directQueue->ExecuteCommandLists(1, lists);

    swapChain->Present(1, 0);

    backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    WaitForDirectQueue();
}

ID3D12Device* Renderer::GetDevice() const {
    return device.Get();
}

ID3D12CommandQueue* Renderer::GetDirectQueue() const {
    return directQueue.Get();
}

ID3D12GraphicsCommandList* Renderer::GetDirectCommandList() const {
    return directCommandList.Get();
}

ID3D12CommandAllocator* Renderer::GetDirectCommandAllocator() const {
    return directCommandAllocator.Get();
}

ID3D12CommandQueue* Renderer::GetCopyQueue() const {
    return copyQueue.Get();
}

ID3D12GraphicsCommandList* Renderer::GetCopyCommandList() const {
    return copyCommandList.Get();
}

ID3D12CommandAllocator* Renderer::GetCopyCommandAllocator() const {
    return copyCommandAllocator.Get();
}

ID3D12Fence* Renderer::GetCopyFence() const {
    return copyFence.Get();
}

UINT64& Renderer::GetCopyFenceValue()
{
    return copyFenceValue;
}

UINT64 Renderer::SignalCopyFence() {
    const UINT64 value = ++copyFenceValue;
    copyQueue->Signal(copyFence.Get(), value);
    return value;
}

void Renderer::WaitCopyFence(UINT64 value) {
    if (copyFence->GetCompletedValue() < value) {
        copyFence->SetEventOnCompletion(value, copyFenceEvent);
        WaitForSingleObject(copyFenceEvent, INFINITE);
    }
}

PipelineStateManager* Renderer::GetPSOManager() const {
    return psoManager.get();
}

RootSignatureManager* Renderer::GetRootSignatureManager() const {
    return rootSignatureManager.get();
}

ShaderManager* Renderer::GetShaderManager() const {
    return shaderManager.get();
}

DescriptorHeapManager* Renderer::GetDescriptorHeapManager() const {
    return descriptorHeapManager.get();
}

TextureManager* Renderer::GetTextureManager() const
{
    return textureManager.get();
}

Camera* Renderer::GetCamera() const {
    return mainCamera.get();
}

void Renderer::SetCamera(std::shared_ptr<Camera> newCamera) {
    mainCamera = newCamera;
}

EnvironmentMaps& Renderer::GetEnvironmentMaps()
{
    return environmentMaps;
}

int Renderer::GetViewportWidth() const {
    return int(viewport.Width);
}

int Renderer::GetViewportHeight() const {
    return int(viewport.Height);
}

ID3D12Resource* Renderer::GetLightingConstantBuffer() const {
    return lightingConstantBuffer.Get();
}

ID3D12Resource* Renderer::GetGlobalConstantBuffer() const {
    return globalConstantBuffer.Get();
}

bool Renderer::InitImGui(HWND hwnd)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // 1) Win32 초기화
    if (!ImGui_ImplWin32_Init(hwnd))
        return false;

    descriptorHeapManager->InitializeImGuiHeaps(device.Get());

    // 2) DX12 초기화
    auto srvHeap = descriptorHeapManager->GetImGuiSrvHeap();
    return ImGui_ImplDX12_Init(
        device.Get(),
        BackBufferCount,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        srvHeap,
        srvHeap->GetCPUDescriptorHandleForHeapStart(),
        srvHeap->GetGPUDescriptorHandleForHeapStart());
}

void Renderer::ImGuiNewFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Renderer::ShutdownImGui()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

UINT Renderer::GetBackBufferIndex() const
{
    return backBufferIndex;
}

D3D12_GPU_DESCRIPTOR_HANDLE Renderer::GetSceneColorSrvHandle() const
{
    return sceneColorSrvHandle.gpu;
}

ID3D12Resource* Renderer::GetSceneColorBuffer() const
{
    return sceneColorBuffer.Get();
}

bool Renderer::InitD3D(HWND hwnd, int width, int height)
{
#ifdef _DEBUG
    // GPU 기반 검증 및 디버그 레이어 활성화
    ComPtr<ID3D12Debug3> debugInterface;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
        debugInterface->SetEnableGPUBasedValidation(TRUE);
        debugInterface->EnableDebugLayer();
    }
#endif

    // 1) 팩토리 및 어댑터 생성
    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapters1(0, &adapter))) {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        ThrowIfFailed(warpAdapter.As(&adapter));
    }

    // 2) 디바이스 생성
    ThrowIfFailed(D3D12CreateDevice(
        adapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device)));

    // 3) Direct(그래픽스) 커맨드 큐 생성
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&directQueue)));
    }

    // 4) Copy(업로드) 커맨드 큐 생성
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&copyQueue)));
    }

    // 5) Copy 커맨드 할당자 & 리스트
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_COPY,
        IID_PPV_ARGS(&copyCommandAllocator)));
    ThrowIfFailed(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_COPY,
        copyCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&copyCommandList)));
    copyCommandList->Close();

    // 6) Copy 큐용 펜스 & 이벤트
    ThrowIfFailed(device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyFence)));
    copyFenceValue = 0;
    copyFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!copyFenceEvent) return false;

    // 7) 스왑체인 생성 (Flip Discard)
    {
        DXGI_SWAP_CHAIN_DESC1 scDesc = {};
        scDesc.BufferCount = BackBufferCount;
        scDesc.Width = width;
        scDesc.Height = height;
        scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        scDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(factory->CreateSwapChainForHwnd(
            directQueue.Get(), hwnd,
            &scDesc, nullptr, nullptr,
            &swapChain1));
        ThrowIfFailed(swapChain1.As(&swapChain));
        backBufferIndex = swapChain->GetCurrentBackBufferIndex();
    }

    // 8) Descriptor Heap 생성
    // SceneColor 는 BackBuffer에도 저장되야하므로 RTV 메모리공간할당은 아래와 같이 수행한다.
    descriptorHeapManager = std::make_unique<DescriptorHeapManager>();
    if (!descriptorHeapManager->Initialize(
        device.Get(),
        10000,   // CBV_SRV_UAV
        16,     // Sampler
        static_cast<UINT>(RtvIndex::RtvCount),       // RTV
        static_cast<UINT>(DsvIndex::DsvCount),       // DSV
        BackBufferCount))    // Back Buffer 수
        return false;


    // 9) RTV 생성
    // 추후에 여기에 GBuffer 도 처리하면 됨
    // Off-screen SceneColor RTV 생성
    {
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Alignment = 0;
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;


        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = texDesc.Format;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,                           // 임시 대신 l-value 변수의 주소
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValue,
            IID_PPV_ARGS(&sceneColorBuffer)));

        // RTV 생성 (SceneColor slot)
        descriptorHeapManager->CreateRenderTargetView(
            device.Get(),
            sceneColorBuffer.Get(),
            static_cast<UINT>(Renderer::RtvIndex::SceneColor));

        // SRV 생성 (PostProcess에서 읽을 수 있도록)
        sceneColorSrvHandle = descriptorHeapManager->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        
        device->CreateShaderResourceView(
            sceneColorBuffer.Get(),   // SRV를 만들 리소스
            nullptr,                  // nullptr 쓰면 리소스 포맷에 맞는 기본 디스크립터
            sceneColorSrvHandle.cpu            // CPU 디스크립터 핸들
        );
    }

    // 10) Swap-Chain BackBuffer용 RTV 생성
    for (UINT i = 0; i < BackBufferCount; ++i) {
        UINT backBufferIndex_ = static_cast<UINT>(RtvIndex::BackBuffer0) + i;

        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])));
        descriptorHeapManager->CreateRenderTargetView(device.Get(),
            backBuffers[i].Get(),
            backBufferIndex_);
    }

    // 11) Depth-Stencil 버퍼 및 DSV 생성
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC depthDesc = {};
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.DepthOrArraySize = 1;
        depthDesc.MipLevels = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&depthStencilBuffer)));

        descriptorHeapManager->CreateDepthStencilView(
            device.Get(),
            depthStencilBuffer.Get(),
            static_cast<UINT>(DsvIndex::DepthStencil));
    }

    // 12) Direct 커맨드 할당자 & 리스트
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&directCommandAllocator)));
    ThrowIfFailed(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        directCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&directCommandList)));
    directCommandList->Close();

    // 13) Direct 큐용 펜스 & 이벤트
    ThrowIfFailed(device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&directFence)));
    directFenceValue = 1;
    directFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!directFenceEvent) return false;

    // 14) Viewport & Scissor 설정
    viewport = { 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    scissorRect = { 0, 0, width, height };

    return true;
}

void Renderer::PopulateCommandList()
{
    // 1) Reset allocator & list
    directCommandAllocator->Reset();
    directCommandList->Reset(directCommandAllocator.Get(), nullptr);


    // 2) 백버퍼 Transition: PRESENT → RENDER_TARGET
    CD3DX12_RESOURCE_BARRIER toRT =
        CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffers[backBufferIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    directCommandList->ResourceBarrier(1, &toRT);

    // 3) RTV/DSV 바인딩 & 클리어
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        descriptorHeapManager->GetRtvHandle(static_cast<UINT>(RtvIndex::BackBuffer0) + backBufferIndex);

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
        descriptorHeapManager->GetDsvHandle(static_cast<UINT>(DsvIndex::DepthStencil));

    const FLOAT clearColor[4] = { 0, 0, 0, 1 };
    directCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    directCommandList->ClearDepthStencilView(dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // 4) 뷰포트/시저 설정
    directCommandList->RSSetViewports(1, &viewport);
    directCommandList->RSSetScissorRects(1, &scissorRect);

    // 5) 렌더 패스 수행
    for (size_t i = 0; i < static_cast<size_t>(PassIndex::Count); ++i)
    {
        renderPasses[i]->Render(this);
    }

    // Imgui 드로우
    {
        ID3D12DescriptorHeap* heaps[] = {
            descriptorHeapManager->GetImGuiSrvHeap(),
            descriptorHeapManager->GetImGuiSamplerHeap()
        };
        directCommandList->SetDescriptorHeaps(_countof(heaps), heaps);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GetDirectCommandList());
    }


    // 6) RENDER_TARGET → PRESENT 전환
    CD3DX12_RESOURCE_BARRIER toPresent =
        CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffers[backBufferIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
    directCommandList->ResourceBarrier(1, &toPresent);


    // 7) 리스트 종료
    directCommandList->Close();
}

void Renderer::WaitForDirectQueue() {
    const UINT64 fenceToWait = directFenceValue;
    directQueue->Signal(directFence.Get(), fenceToWait);
    directFenceValue++;

    if (directFence->GetCompletedValue() < fenceToWait) {
        directFence->SetEventOnCompletion(fenceToWait, directFenceEvent);
        WaitForSingleObject(directFenceEvent, INFINITE);
    }

    backBufferIndex = swapChain->GetCurrentBackBufferIndex();
}

void Renderer::UpdateGlobalTime(float seconds) {
    globalData.time = seconds;
    memcpy(mappedGlobalPtr, &globalData, sizeof(globalData));
}