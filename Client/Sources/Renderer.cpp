#include "Renderer.h"
#include <stdexcept>

Renderer::Renderer() {}

Renderer::~Renderer() {
    Cleanup();
}

bool Renderer::Init(HWND hwnd, int width, int height) {
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
        { L"PbrPixelShader",       L"Shaders/PbrPixelShader.hlsl",       "PSMain", "ps_5_1" }
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

    descriptorHeapManager = std::make_unique<DescriptorHeapManager>();
    if (!descriptorHeapManager->Initialize(
        device.Get(),
        60000,   // CBV_SRV_UAV
        256,     // Sampler
        0,       // RTV
        0,       // DSV
        FrameCount))
        return false;

    // Setup camera
    mainCamera = std::make_shared<Camera>();
    mainCamera->SetPosition({ 0, 0, -30 });
    mainCamera->SetPerspective(XM_PIDIV4, float(width) / height, 0.1f, 100.f);

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
        lightingData.cameraWorld = { 0.f, 0.f, -5.f };

        Light directionalLight{};
        directionalLight.strength = { 1.f, 1.f, 1.f };
        directionalLight.direction = { 0.f, -1.f, 0.f };
        directionalLight.specPower = 32.f;
        directionalLight.type = 0; // 0 = Directional
        directionalLight.color = { 1.f, 1.f, 1.f };

        lightingData.lights[0] = directionalLight;
        memcpy(mappedLightingPtr, &lightingData, sizeof(lightingData));
    }

    // Sampler (s0)
    {
        auto samplerHandle = descriptorHeapManager->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        samplerGpuHandle = samplerHandle.gpu; // root slot 5

        D3D12_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

        device->CreateSampler(&samplerDesc, samplerHandle.cpu);
    }

    return true;
}

void Renderer::Cleanup() {
    WaitForPreviousFrame();
    if (directFenceEvent)
        CloseHandle(directFenceEvent);
}

void Renderer::AddGameObject(std::shared_ptr<GameObject> obj) {
    gameObjects.push_back(obj);
}

void Renderer::RemoveGameObject(std::shared_ptr<GameObject> obj) {
    gameObjects.erase(
        std::remove(gameObjects.begin(), gameObjects.end(), obj),
        gameObjects.end());
}

void Renderer::Update(float deltaTime) {
    lightingData.cameraWorld = mainCamera->GetPosition();
    memcpy(mappedLightingPtr, &lightingData, sizeof(lightingData));
    for (auto& obj : gameObjects)
        obj->Update(deltaTime);
}

void Renderer::Render() {
    PopulateCommandList();

    ID3D12CommandList* lists[] = { directCommandList.Get() };
    directQueue->ExecuteCommandLists(1, lists);

    swapChain->Present(1, 0);

    frameIndex = swapChain->GetCurrentBackBufferIndex();

    WaitForPreviousFrame();
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

Camera* Renderer::GetCamera() const {
    return mainCamera.get();
}

void Renderer::SetCamera(std::shared_ptr<Camera> newCamera) {
    mainCamera = newCamera;
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

D3D12_GPU_DESCRIPTOR_HANDLE Renderer::GetSamplerGpuHandle() const
{
    return samplerGpuHandle;
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
        scDesc.BufferCount = FrameCount;
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
        frameIndex = swapChain->GetCurrentBackBufferIndex();
    }

    // 8) RTV / DSV 힙 생성
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
        rtvDesc.NumDescriptors = FrameCount;
        rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap)));
        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
        dsvDesc.NumDescriptors = 1;
        dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&dsvHeap)));
    }

    // 9) 백버퍼용 RTV 생성
    {
        auto rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < FrameCount; ++i) {
            ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
            device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    // 10) Depth-Stencil 버퍼 및 DSV 생성
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

        device->CreateDepthStencilView(
            depthStencilBuffer.Get(),
            nullptr,
            dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // 11) Direct 커맨드 할당자 & 리스트
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

    // 12) Direct 큐용 펜스 & 이벤트
    ThrowIfFailed(device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&directFence)));
    directFenceValue = 1;
    directFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!directFenceEvent) return false;

    // 13) Viewport & Scissor 설정
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
            renderTargets[frameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    directCommandList->ResourceBarrier(1, &toRT);

    // 3) RTV/DSV 바인딩 & 클리어
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += frameIndex * rtvDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

    directCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    const FLOAT clearColor[4] = { 0, 0, 0, 1 };
    directCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    directCommandList->ClearDepthStencilView(dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // 4) 뷰포트/시저 설정
    directCommandList->RSSetViewports(1, &viewport);
    directCommandList->RSSetScissorRects(1, &scissorRect);

    // 5) 각 게임 오브젝트 렌더 호출
    for (auto& obj : gameObjects) {
        obj->Render(this);
    }

    // 6) RENDER_TARGET → PRESENT 전환
    CD3DX12_RESOURCE_BARRIER toPresent =
        CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
    directCommandList->ResourceBarrier(1, &toPresent);

    // 7) 리스트 종료
    directCommandList->Close();
}

void Renderer::WaitForPreviousFrame() {
    const UINT64 fenceToWait = directFenceValue;
    directQueue->Signal(directFence.Get(), fenceToWait);
    directFenceValue++;

    if (directFence->GetCompletedValue() < fenceToWait) {
        directFence->SetEventOnCompletion(fenceToWait, directFenceEvent);
        WaitForSingleObject(directFenceEvent, INFINITE);
    }

    frameIndex = swapChain->GetCurrentBackBufferIndex();
}

void Renderer::UpdateGlobalTime(float seconds) {
    globalData.time = seconds;
    memcpy(mappedGlobalPtr, &globalData, sizeof(globalData));
}