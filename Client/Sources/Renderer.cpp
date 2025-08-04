#include "Renderer.h"
#include "RenderPass/ShadowMapPass.h"
#include "RenderPass/ForwardOpaquePass.h"
#include "RenderPass/ForwardTransparentPass.h"
#include "RenderPass/PostProcessPass.h"
#include "Lights/DirectionalLight.h"
#include "Lights/PointLight.h"
#include "Lights/SpotLight.h"
#include "ThreadPool.h"
#include <stdexcept>

Renderer::Renderer()
{
    threadPool = std::make_unique<ThreadPool>(std::thread::hardware_concurrency());
}


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


    // LightingManager 초기화
    {
        lightingManager = std::make_unique<LightingManager>();

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
    }

    descriptorHeapManager->CreateLinearWrapSampler(device.Get());
    descriptorHeapManager->CreateLinearClampSampler(device.Get());


    frameResources.clear();
    frameResources.reserve(BackBufferCount);
    for (UINT i = 0; i < BackBufferCount; ++i) {
        frameResources.emplace_back(std::make_unique<FrameResource>(
            device.Get(),
            descriptorHeapManager.get(),
            static_cast<UINT>(/*GameObject 최대 수=*/1000),
            GetViewportWidth(),
            GetViewportHeight()
        ));
    }

    currentFrameIndex = 0;
    currentFrameResource = frameResources[currentFrameIndex].get();


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

    currentFrameIndex = (currentFrameIndex + 1) % BackBufferCount;

    currentFrameResource = frameResources[currentFrameIndex].get();

    // currentFrmaeResource의 fence를 통해 currentFrameResource가 제출한 GPU작업이 끝났는지를 판단하고 대기한다.
    UINT64 lastCompleted = directFence->GetCompletedValue();
    if (currentFrameResource->fenceValue > lastCompleted) {
        ThrowIfFailed(directFence->SetEventOnCompletion(
            currentFrameResource->fenceValue,
            directFenceEvent
        ));
        WaitForSingleObject(directFenceEvent, INFINITE);
    }

    lightingManager->Update(this);

    for (UINT i = 0; i < gameObjects.size(); ++i) {
        gameObjects[i]->Update(deltaTime, this, i);
    }

    for (auto& pass : renderPasses) {
        pass->Update(deltaTime, this);
    }
}

void Renderer::Render() {

    if (IsMultithreadedRenderingEnabled()) {
        RenderMultiThreaded();
    }
    else {
        RenderSingleThreaded();
    }
}

void Renderer::RenderSingleThreaded()
{
    RecordCommandList_SingleThreaded();

    FrameResource* currentFrameResource = frameResources[currentFrameIndex].get();
    ID3D12CommandList* commandLists[] = { currentFrameResource->commandList.Get() };

    directQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    swapChain->Present(1, 0);

    backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    GetCurrentFrameResource()->fenceValue = directFenceValue;
    directQueue->Signal(directFence.Get(), directFenceValue);
    ++directFenceValue;
}

void Renderer::RenderMultiThreaded()
{

}

ID3D12Device* Renderer::GetDevice() const {
    return device.Get();
}

ID3D12CommandQueue* Renderer::GetDirectQueue() const {
    return directQueue.Get();
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

ThreadPool* Renderer::GetThreadPool()
{
    return threadPool.get();
}

ID3D12CommandAllocator* Renderer::GetThreadCommandAllocator(UINT index)
{   
    return threadCommandAllocators[index].Get();
}

ID3D12GraphicsCommandList* Renderer::GetThreadCommandList(UINT index)
{
    return threadCommandLists[index].Get();
}

void Renderer::AddRecordedCommandList(ID3D12CommandList* commandList)
{
    recordedCommandLists.push_back(commandList);
}

void Renderer::ClearRecordedCommandLists()
{
    recordedCommandLists.clear();
}

const std::vector<ID3D12CommandList*>& Renderer::GetRecordedCommandLists() const
{
    return recordedCommandLists;
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

const std::vector<DescriptorHandle>& Renderer::GetSwapChainRtvs() const
{
    return swapChainRtvs;
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

    descriptorHeapManager->InitializeImGuiDescriptorHeaps(device.Get());

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

FrameResource* Renderer::GetCurrentFrameResource()
{
    return currentFrameResource;
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

    // 팩토리 및 어댑터 생성
    ComPtr<IDXGIFactory4> factory;
    THROW_IF_FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapters1(0, &adapter))) {
        ComPtr<IDXGIAdapter> warpAdapter;
        THROW_IF_FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        THROW_IF_FAILED(warpAdapter.As(&adapter));
    }

    // 디바이스 생성
    THROW_IF_FAILED(D3D12CreateDevice(
        adapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device)));

    // Direct 커맨드 큐 생성
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        THROW_IF_FAILED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&directQueue)));
    }

    // Copy 커맨드 큐 생성
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

    // Copy 큐용 펜스 & 이벤트
    THROW_IF_FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyFence)));
    copyFenceValue = 0;
    copyFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!copyFenceEvent) return false;

    // 스왑체인 생성 (Flip Discard)
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

    // Descriptor Heap 생성
    descriptorHeapManager = std::make_unique<DescriptorHeapManager>();
    if (!descriptorHeapManager->Initialize(
        device.Get(),
        /*CBV_SRV_UAV*/ 10000,
        /*Sampler*/      128,
        /*RTV*/          100,
        /*DSV*/          300,
        /*backBuffers*/  BackBufferCount))
    {
        return false;
    }

    // Swap-Chain BackBuffer용 RTV 생성
    swapChainRtvs.resize(BackBufferCount);
    for (UINT i = 0; i < BackBufferCount; ++i) {
        THROW_IF_FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])));

        // RTV 슬롯 할당 + 뷰 생성
        swapChainRtvs[i] = descriptorHeapManager->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        descriptorHeapManager->CreateRenderTargetView(
            device.Get(),
            backBuffers[i].Get(),
            swapChainRtvs[i].index);
    }


    // Direct 큐용 펜스 & 이벤트
    THROW_IF_FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&directFence)));
    directFenceValue = 1;
    directFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!directFenceEvent) return false;

    // Viewport & Scissor 설정
    viewport = { 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    scissorRect = { 0, 0, width, height };

    return true;
}

void Renderer::RecordCommandList_SingleThreaded()
{
    FrameResource* currentFrameResource = frameResources[currentFrameIndex].get();

    currentFrameResource->Reset();

    ID3D12GraphicsCommandList* commandList = currentFrameResource->commandList.Get();

    // 백버퍼 Transition: PRESENT → RENDER_TARGET
    CD3DX12_RESOURCE_BARRIER toRenderTarget =
        CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffers[backBufferIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &toRenderTarget);


    for (size_t i = 0; i < static_cast<size_t>(PassIndex::Count); ++i)
    {
        renderPasses[i]->RenderSingleThreaded(this);
    }

    // Imgui 드로우
    {
        ID3D12DescriptorHeap* heaps[] = {
            descriptorHeapManager->GetImGuiSrvHeap(),
            descriptorHeapManager->GetImGuiSamplerHeap()
        };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    }


    // RENDER_TARGET → PRESENT 전환
    CD3DX12_RESOURCE_BARRIER toPresent =
        CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffers[backBufferIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &toPresent);

    // 커맨드 리스트 종료
    commandList->Close();
}

void Renderer::WaitForDirectQueue() {

    const UINT64 fenceToWait = directFenceValue;

    // Signal은 바로 수행되지는 않고, directQueue가 모든 작업을 마치면 Signal 을 수행시켜 
    // directFence 값을 지정한 fenceToWait로 바꾼다.

    directQueue->Signal(directFence.Get(), fenceToWait);

    // 다음 번 Signal에서 중복 없이 새로운 펜스 값을 쓰기 위한 작업
    directFenceValue++;     


    // 만약 if문 수행전에 작업이 끝나면 Wait도 안하고 바로 프레임으로 넘어간다.
    if (directFence->GetCompletedValue() < fenceToWait) {
        directFence->SetEventOnCompletion(fenceToWait, directFenceEvent);
        WaitForSingleObject(directFenceEvent, INFINITE);
    }
}

void Renderer::UpdateGlobalTime(float seconds) {

    CB_Global globalConstants{};
    globalConstants.time = seconds;

    FrameResource* currentFrame = GetCurrentFrameResource();
    assert(currentFrame && currentFrame->cbGlobal && "FrameResource or cbGlobal is null");
    currentFrame->cbGlobal->CopyData(0, globalConstants);
}

bool Renderer::IsMultithreadedRenderingEnabled() const
{
    return useMultithreadedRendering;
}
