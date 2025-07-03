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
    if (!rootSignatureManager->InitializeDescs())
        return false;

    shaderManager = std::make_unique<ShaderManager>(device.Get());

    std::vector<ShaderCompileDesc> shaderDescs = {
        {L"TriangleVertexShader", L"Shaders/TriangleVertexShader.hlsl", "VSMain", "vs_5_0"},
        {L"TrianglePixelShader", L"Shaders/TrianglePixelShader.hlsl", "PSMain", "ps_5_0"},
        {L"PhongVertexShader",    L"Shaders/PhongVertexShader.hlsl", "VSMain", "vs_5_0"},
        {L"PhongPixelShader",     L"Shaders/PhongPixelShader.hlsl",  "PSMain", "ps_5_0"}
    };

    if (!shaderManager->CompileAll(shaderDescs))
        return false;


    psoManager = std::make_unique<PipelineStateManager>(this);

    if (!psoManager->InitializePSOs())
        return false;


    descriptorHeapManager = std::make_unique<DescriptorHeapManager>();
    if (!descriptorHeapManager->Initialize(
        device.Get(),
        60'000,   // CBV_SRV_UAV
        256,      // Sampler
        0,        // RTV   (Renderer�� ����� ó��)
        0,        // DSV   (Renderer�� ����� ó��)
        FrameCount))
        return false;


    // Setup camera
    mainCamera = std::make_shared<Camera>();
    mainCamera->SetPosition({ 0,0,-30 });
    mainCamera->SetPerspective(XM_PIDIV4, float(width) / height, 0.1f, 100.f);


    // �۷ι�-Ÿ�� CB (b3)
    {
        CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC   desc = CD3DX12_RESOURCE_DESC::Buffer(256);

        if (FAILED(device->CreateCommittedResource(
            &props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&globalConstantBuffer))))
            return false;

        globalConstantBuffer->Map(0, nullptr,
            reinterpret_cast<void**>(&mappedGlobalPtr));
    }

    // ������ CB (b1)
    {
        CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC   desc = CD3DX12_RESOURCE_DESC::Buffer(256);

        if (FAILED(device->CreateCommittedResource(
            &props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&lightingConstantBuffer))))
            return false;

        lightingConstantBuffer->Map(0, nullptr,
            reinterpret_cast<void**>(&mappedLightingPtr));

        /* �⺻ ������ �� ���� */
        lightingData = {};
        lightingData.cameraWorld = { 0.f, 0.f, 0.f };

        Light dirLight{};
        dirLight.strength = { 1.f, 1.f, 1.f };
        dirLight.direction = { 0.f,-1.f, 0.f };
        dirLight.specPower = 32.f;
        dirLight.type = 0;              // 0 = Directional
        dirLight.color = { 1.f,1.f,1.f };

        lightingData.lights[0] = dirLight;
        memcpy(mappedLightingPtr, &lightingData, sizeof(lightingData));

    }


    // Sampler (s0) 1�� ���� & �� ���� Ȯ��
    {
        // 1) �� ���� 1�� �Ҵ�
        auto samplerHandle = descriptorHeapManager->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        samplerGpuHandle = samplerHandle.gpu;     // root slot 5���� ����� GPU �ڵ�

        // 2) ���÷� ��ũ���� �ۼ�
        D3D12_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampDesc.MaxLOD = D3D12_FLOAT32_MAX;

        device->CreateSampler(&sampDesc, samplerHandle.cpu);
    }

    return true;
}

void Renderer::Cleanup() {
    WaitForPreviousFrame();
    if (fenceEvent) CloseHandle(fenceEvent);
}

void Renderer::AddGameObject(std::shared_ptr<GameObject> obj) {
    gameObjects.push_back(obj);
}

void Renderer::RemoveGameObject(std::shared_ptr<GameObject> obj) {
    gameObjects.erase(
        std::remove(gameObjects.begin(), gameObjects.end(), obj),
        gameObjects.end());
}

void Renderer::Update(float deltaTime)
{
    lightingData.cameraWorld = mainCamera->GetPosition();
    memcpy(mappedLightingPtr, &lightingData, sizeof(lightingData));


    for (auto& gameObject : gameObjects) {
        gameObject->Update(deltaTime);
    }
}

void Renderer::Render() {
    // ����� ���Ͻ����� ���������� ���ư�

    // 1) Ŀ�ǵ� ���
    PopulateCommandList();

    // 2) ��ϵ� Ŀ�ǵ� ����
    ID3D12CommandList* lists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(1, lists);

    // 3) ȭ�鿡 ǥ��
    swapChain->Present(1, 0);

    // 4) ������ ����� ����� �ε��� ���
    frameIndex = swapChain->GetCurrentBackBufferIndex();

    // 5) GPU �Ϸ� ���
    WaitForPreviousFrame();
}

ID3D12Device* Renderer::GetDevice() const
{
    return device.Get();
}

ID3D12CommandQueue* Renderer::GetCommandQueue() const
{
    return commandQueue.Get();
}

ID3D12GraphicsCommandList* Renderer::GetCommandList() const
{
    return commandList.Get();
}

ID3D12CommandAllocator* Renderer::GetCommandAllocator() const
{
    return commandAllocator.Get();
}

ID3D12CommandAllocator* Renderer::GetUploadCommandAllocator() const
{
    return uploadCommandAllocator.Get();
}

ID3D12GraphicsCommandList* Renderer::GetUploadCommandList() const
{
    return uploadCommandList.Get();
}

PipelineStateManager* Renderer::GetPSOManager() const
{
    return psoManager.get();
}

RootSignatureManager* Renderer::GetRootSignatureManager() const
{
    return rootSignatureManager.get();
}

ShaderManager* Renderer::GetShaderManager() const
{
    return shaderManager.get();
}

DescriptorHeapManager* Renderer::GetDescriptorHeapManager() const
{
    return descriptorHeapManager.get();
}

ID3D12CommandQueue* Renderer::GetUploadQueue() const
{
    return uploadQueue.Get();
}

ID3D12Fence* Renderer::GetUploadFence() const
{
    return uploadFence.Get();
}

UINT64 Renderer::IncrementUploadFenceValue()
{
    return ++uploadFenceValue;
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

ID3D12Resource* Renderer::GetLightingConstantBuffer() const
{
    return lightingConstantBuffer.Get();
}

ID3D12Resource* Renderer::GetGlobalConstantBuffer() const
{
    return globalConstantBuffer.Get();
}

bool Renderer::InitD3D(HWND hwnd, int width, int height)
{

#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
        }
    }
#endif


    // 1. D3D12 ����̽� �� ���丮 ����
    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapters1(0, &adapter))) {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        ThrowIfFailed(warpAdapter.As(&adapter));
    }
    ThrowIfFailed(D3D12CreateDevice(
        adapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device)
    ));

    // 2. Ŀ�ǵ� ť ����
    D3D12_COMMAND_QUEUE_DESC cqDesc = {};
    cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue)));

    // 2.1 ���ε� ���� Ŀ�ǵ� ť ���� (Copy ����)
    D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {};
    copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    copyQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&uploadQueue)));

    // 2.2 ���ε�� Ŀ�ǵ� �Ҵ��� & ����Ʈ ����
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_COPY,
        IID_PPV_ARGS(&uploadCommandAllocator)));
    ThrowIfFailed(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_COPY,
        uploadCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&uploadCommandList)));
    uploadCommandList->Close();

    // 2.3 ���ε� ���� �潺 ����
    ThrowIfFailed(device->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&uploadFence)));
    uploadFenceValue = 0;


    // 3. ����ü�� ���� (Flip ���)
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
        commandQueue.Get(),
        hwnd,
        &scDesc,
        nullptr, nullptr,
        &swapChain1
    ));
    ThrowIfFailed(swapChain1.As(&swapChain));
    frameIndex = swapChain->GetCurrentBackBufferIndex();

    // 4. Descriptor Heap ���� (RTV, DSV)
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));
    }

    // 5. ����ۿ� RTV ����
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < FrameCount; i++) {
            ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
            device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    // 6. Depth-Stencil ���� ���� �� DSV ����
    {
        // D3D12_HEAP_PROPERTIES ����
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC depthDesc = {};
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.DepthOrArraySize = 1;
        depthDesc.MipLevels = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
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
            IID_PPV_ARGS(&depthStencilBuffer)
        ));
        device->CreateDepthStencilView(
            depthStencilBuffer.Get(),
            nullptr,
            dsvHeap->GetCPUDescriptorHandleForHeapStart()
        );
    }

    // 7. Ŀ�ǵ� �Ҵ��� & Ŀ�ǵ� ����Ʈ ����
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator)
    ));

    ThrowIfFailed(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&commandList)
    ));
    commandList->Close(); 

    // 8. Fence & �̺�Ʈ ����
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    fenceValue = 1;
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!fenceEvent) return false;

    // 9. Viewport & Scissor ����
    viewport = { 0.0f, 0.0f, (FLOAT)width, (FLOAT)height, 0.0f, 1.0f };
    scissorRect = { 0, 0, width, height };

    return true;
}


void Renderer::PopulateCommandList() {
    // 1) Reset allocator & list
    commandAllocator->Reset();
    commandList->Reset(commandAllocator.Get(), nullptr);


    /* �� �� �� ���ε� (CBV_SRV_UAV + SAMPLER) */
    ID3D12DescriptorHeap* heaps[] = {
        descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    commandList->SetDescriptorHeaps(2, heaps);


    // ��Ʈ ���� ����!
    commandList->SetGraphicsRootSignature(
        rootSignatureManager->Get(L"PhongRS"));   // �Ǵ� ���� RS


    /* ������ ���� Root-CBV (b1, b3)  */
    commandList->SetGraphicsRootConstantBufferView(
        1, lightingConstantBuffer->GetGPUVirtualAddress());   // b1

    commandList->SetGraphicsRootConstantBufferView(
        3, globalConstantBuffer->GetGPUVirtualAddress());     // b3

    /* Sampler ���̺� (root slot 5)  */
    commandList->SetGraphicsRootDescriptorTable(
        5, samplerGpuHandle);



    // 2) ����� Transition: PRESENT �� RENDER_TARGET
    CD3DX12_RESOURCE_BARRIER barrierToRenderTarget =
        CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrierToRenderTarget);

    // 3) RTV/DSV ���ε� & Ŭ����
    auto rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += frameIndex * rtvDescriptorSize;
    auto dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    const FLOAT clearColor[] = { 0,0,0,1 };
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsv,
        D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


    // 4) ����Ʈ/����
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // 5) �� ���� ������Ʈ ��ο���
    for (auto& obj : gameObjects) {
        obj->Render(this);
    }

    // 6) ����� Transition: RENDER_TARGET �� PRESENT
    CD3DX12_RESOURCE_BARRIER barrierToPresent =
        CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrierToPresent);

    // 7) Ŀ�ǵ� ����Ʈ ������
    commandList->Close();
}

void Renderer::WaitForPreviousFrame() {
    const UINT64 currentFence = fenceValue;
    commandQueue->Signal(fence.Get(), currentFence);
    fenceValue++;

    if (fence->GetCompletedValue() < currentFence) {
        fence->SetEventOnCompletion(currentFence, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

void Renderer::UpdateGlobalTime(float timeSeconds)
{
    globalData.time = timeSeconds;
    memcpy(mappedGlobalPtr, &globalData, sizeof(globalData));
}
