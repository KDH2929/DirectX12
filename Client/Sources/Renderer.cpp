#include "Renderer.h"
#include "RenderPass/ShadowMapPass.h"
#include "RenderPass/ForwardOpaquePass.h"
#include "RenderPass/ForwardTransparentPass.h"
#include "RenderPass/PostProcessPass.h"
#include "Lights/DirectionalLight.h"
#include "Lights/PointLight.h"
#include "Lights/SpotLight.h"
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
        { L"TriangleVS", L"Shaders/TriangleVS.hlsl", "VSMain", "vs_5_0" },
        { L"TrianglePS",  L"Shaders/TrianglePS.hlsl",  "PSMain", "ps_5_0" },
        { L"PhongVS",    L"Shaders/PhongVS.hlsl",    "VSMain", "vs_5_1" },
        { L"PhongPS",     L"Shaders/PhongPS.hlsl",     "PSMain", "ps_5_1" },
        { L"PbrVS",      L"Shaders/PbrVS.hlsl",      "VSMain", "vs_5_1" },
        { L"PbrPS",       L"Shaders/PbrPS.hlsl",       "PSMain", "ps_5_1" },

        { L"SkyboxVS",     L"Shaders/SkyboxVS.hlsl",     "VSMain", "vs_5_0" },
        { L"SkyboxPS",      L"Shaders/SkyboxPS.hlsl",      "PSMain", "ps_5_0" },
        
        {L"DebugNormalVS", L"Shaders/DebugNormalShader.hlsl", "VSMain", "vs_5_0"},
        {L"DebugNormalGS", L"Shaders/DebugNormalShader.hlsl", "GSMain", "gs_5_0"},
        {L"DebugNormalPS", L"Shaders/DebugNormalShader.hlsl", "PSMain", "ps_5_0"},

        { L"OutlinePostEffectVS",   L"Shaders/OutlinePostEffect.hlsl",  "VSMain", "vs_5_0" },
        { L"OutlinePostEffectPS",   L"Shaders/OutlinePostEffect.hlsl",  "PSMain", "ps_5_0" },

        { L"ToneMappingPostEffectVS", L"Shaders/ToneMappingPostEffect.hlsl", "VSMain", "vs_5_0" },
        { L"ToneMappingPostEffectPS", L"Shaders/ToneMappingPostEffect.hlsl", "PSMain", "ps_5_0" },

        { L"ShadowMapPassVS", L"Shaders/ShadowMapPass.hlsl", "VSMain", "vs_5_0" },
        { L"ShadowMapPassPS", L"Shaders/ShadowMapPass.hlsl", "PSMain", "ps_5_0" },

        { L"VolumetricCloudVS",    L"Shaders/VolumetricCloudVS.hlsl",   "VSMain", "vs_5_0" },
        { L"VolumetricCloudGS",    L"Shaders/VolumetricCloudGS.hlsl",   "GSMain", "gs_5_0" },
        { L"VolumetricCloudPS",    L"Shaders/VolumetricCloudPS.hlsl",   "PSMain", "ps_5_0" },
    
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

    // Global Constant Buffer (b3)
    {
        CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(256);
        THROW_IF_FAILED(device->CreateCommittedResource(
            &props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&globalConstantBuffer)));
        globalConstantBuffer->Map(0, nullptr,
            reinterpret_cast<void**>(&mappedGlobalPtr));
    }



    // LightingManager 초기화
    {
        lightingManager = std::make_unique<LightingManager>(device.Get());

        XMFLOAT3 cameraPosition = mainCamera->GetPosition();
        XMFLOAT3 cameraForward = mainCamera->GetForwardVector();

        // Directional Light
        auto directionalLight = std::make_shared<DirectionalLight>();
        directionalLight->SetStrength(1.0f);
        directionalLight->SetColor({ 1.0f, 1.0f, 1.0f });
        directionalLight->SetDirection({ 0.0f, -1.0f, 0.0f });
        directionalLight->SetAttenuation(1.0f, 0.0f, 0.0f);

        // Point Light
        auto pointLight = std::make_shared<PointLight>();
        pointLight->SetStrength(2.0f);
        pointLight->SetColor({ 1.0f, 0.95f, 0.85f });
        pointLight->SetPosition({ cameraPosition.x, cameraPosition.y + 2.0f, cameraPosition.z });
        pointLight->SetAttenuation(1.0f, 0.09f, 0.032f);

        // Spot Light
        auto spotLight = std::make_shared<SpotLight>();
        spotLight->SetStrength(3.0f);
        spotLight->SetColor({ 1.0f, 1.0f, 1.0f });
        spotLight->SetPosition({
            cameraPosition.x + cameraForward.x * 3.0f,
            cameraPosition.y + cameraForward.y * 3.0f,
            cameraPosition.z + cameraForward.z * 3.0f
            });
        spotLight->SetDirection(cameraForward);
        spotLight->SetCutoffAngles(
            XMConvertToRadians(12.5f),
            XMConvertToRadians(17.5f)
        );
        spotLight->SetAttenuation(1.0f, 0.09f, 0.032f);

        lightingManager->ClearLights();
        lightingManager->AddLight(directionalLight);
        lightingManager->AddLight(pointLight);
        lightingManager->AddLight(spotLight);

        lightingManager->WriteLightingBuffer();
    }
    descriptorHeapManager->CreateWrapSampler(device.Get());
    descriptorHeapManager->CreateClampSampler(device.Get());


    // 렌더패스 초기화
    renderPasses[static_cast<size_t>(PassIndex::ShadowMap)] = std::make_unique<ShadowMapPass>();
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

    lightingManager->Update(mainCamera.get());

    for (auto& object : gameObjects) {
        object->Update(deltaTime);
    }

    for (auto& pass : renderPasses) {
        pass->Update(deltaTime);
    }
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

LightingManager* Renderer::GetLightingManager() const
{
    return lightingManager.get();
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

ID3D12Resource* Renderer::GetGlobalConstantBuffer() const
{
    return globalConstantBuffer.Get();
}

int Renderer::GetViewportWidth() const {
    return int(viewport.Width);
}

int Renderer::GetViewportHeight() const {
    return int(viewport.Height);
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
    return sceneColorSrvHandle.gpuHandle;
}

ID3D12Resource* Renderer::GetSceneColorBuffer() const
{
    return sceneColorBuffer.Get();
}

const std::array<ShadowMap, MAX_SHADOW_DSV_COUNT>& Renderer::GetShadowMaps() const
{
    return shadowMaps;
}

bool Renderer::InitD3D(HWND hwnd, int width, int height)
{
#ifdef _DEBUG
    // GPU 기반 검증 및 디버그 레이어활성화
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
        debugInterface->SetEnableGPUBasedValidation(FALSE);
        debugInterface->EnableDebugLayer();
    }
#endif

    // 1) 팩토리 및 어댑터 생성
    ComPtr<IDXGIFactory4> factory;
    THROW_IF_FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapters1(0, &adapter))) {
        ComPtr<IDXGIAdapter> warpAdapter;
        THROW_IF_FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        THROW_IF_FAILED(warpAdapter.As(&adapter));
    }

    // 2) 디바이스 생성
    THROW_IF_FAILED(D3D12CreateDevice(
        adapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device)));


    // 3) Direct(그래픽스) 커맨드 큐 생성
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        THROW_IF_FAILED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&directQueue)));
    }

    // 4) Copy(업로드) 커맨드 큐 생성
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        THROW_IF_FAILED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&copyQueue)));
    }

    // 5) Copy 커맨드 할당자 & 리스트
    THROW_IF_FAILED(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_COPY,
        IID_PPV_ARGS(&copyCommandAllocator)));
    THROW_IF_FAILED(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_COPY,
        copyCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&copyCommandList)));
    copyCommandList->Close();

    // 6) Copy 큐용 펜스 & 이벤트
    THROW_IF_FAILED(device->CreateFence(
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
        THROW_IF_FAILED(factory->CreateSwapChainForHwnd(
            directQueue.Get(), hwnd,
            &scDesc, nullptr, nullptr,
            &swapChain1));
        THROW_IF_FAILED(swapChain1.As(&swapChain));
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
        // Format 을 Float16으로 해야 HDR 이미지 처리가 가능
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Alignment = 0;
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
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
        THROW_IF_FAILED(device->CreateCommittedResource(
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
            sceneColorSrvHandle.cpuHandle            // CPU 디스크립터 핸들
        );
    }

    // 10) Swap-Chain BackBuffer용 RTV 생성
    for (UINT i = 0; i < BackBufferCount; ++i) {
        UINT backBufferIndex_ = static_cast<UINT>(RtvIndex::BackBuffer0) + i;

        THROW_IF_FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])));
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

        THROW_IF_FAILED(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&depthStencilBuffer)));

        descriptorHeapManager->CreateDepthStencilView(
            device.Get(),
            depthStencilBuffer.Get(),
            nullptr,        // 디폴트값 사용
            static_cast<UINT>(DsvIndex::DepthStencil));
    }


    // 12) ShadowMap DSV 및 SRV 생성
    for (int i = 0; i < MAX_SHADOW_DSV_COUNT; ++i)
    {
        // 리소스 생성
        D3D12_RESOURCE_DESC shadowDesc = {};
        shadowDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        shadowDesc.Width = SHADOW_MAP_WIDTH;
        shadowDesc.Height = SHADOW_MAP_HEIGHT;
        shadowDesc.DepthOrArraySize = 1;
        shadowDesc.MipLevels = 1;
        shadowDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;  // SRV 로도 사용하기 위함
        shadowDesc.SampleDesc.Count = 1;
        shadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  // DSV에서 사용할 포맷
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        THROW_IF_FAILED(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &shadowDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&shadowMaps[i].depthBuffer)
        ));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 또는 DXGI_FORMAT_D32_FLOAT, 용도에 따라
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;


        // DSV 등록
        descriptorHeapManager->CreateDepthStencilView(
            device.Get(),
            shadowMaps[i].depthBuffer.Get(),
            &dsvDesc,
            static_cast<UINT>(DsvIndex::ShadowMap0) + i);

        shadowMaps[i].dsvHandle = descriptorHeapManager->GetDsvHandle(static_cast<UINT>(DsvIndex::ShadowMap0) + i);

        // SRV 등록 (shader에서 샘플링 용도)
        shadowMaps[i].srvHandle = descriptorHeapManager->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;  // 반드시 이 포맷 사용해야 SRV로 가능
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        device->CreateShaderResourceView(
            shadowMaps[i].depthBuffer.Get(),
            &srvDesc,
            shadowMaps[i].srvHandle.cpuHandle);
    }


    // 13) Direct 커맨드 할당자 & 리스트
    THROW_IF_FAILED(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&directCommandAllocator)));
    THROW_IF_FAILED(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        directCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&directCommandList)));
    directCommandList->Close();

    // 14) Direct 큐용 펜스 & 이벤트
    THROW_IF_FAILED(device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&directFence)));
    directFenceValue = 1;
    directFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!directFenceEvent) return false;

    // 15) Viewport & Scissor 설정
    viewport = { 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    scissorRect = { 0, 0, width, height };

    return true;
}

void Renderer::PopulateCommandList()
{
    // Reset allocator & list
    directCommandAllocator->Reset();
    directCommandList->Reset(directCommandAllocator.Get(), nullptr);


    // 백버퍼 Transition: PRESENT → RENDER_TARGET
    CD3DX12_RESOURCE_BARRIER toRT =
        CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffers[backBufferIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    directCommandList->ResourceBarrier(1, &toRT);


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


    // RENDER_TARGET → PRESENT 전환
    CD3DX12_RESOURCE_BARRIER toPresent =
        CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffers[backBufferIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
    directCommandList->ResourceBarrier(1, &toPresent);


    // 리스트 종료
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