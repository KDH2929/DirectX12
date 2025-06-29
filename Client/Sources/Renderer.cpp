#include "Renderer.h"
#include <stdexcept>

inline void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        throw std::runtime_error("HRESULT failed");
    }
}

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
    if (!shaderManager->Init(GetDevice()) ||
        !shaderManager->CompileAll({
            // Triangle shaders
            {L"TriangleVertexShader", L"Shaders/TriangleVertexShader.hlsl", "VSMain", "vs_5_0"},
            {L"TrianglePixelShader", L"Shaders/TrianglePixelShader.hlsl", "PSMain", "ps_5_0"}
            })) {
        return false;
    }

    psoManager = std::make_unique<PipelineStateManager>(this);

    if (!psoManager->InitializePSOs())
        return false;

    // Setup camera
    mainCamera = std::make_shared<Camera>();
    mainCamera->SetPosition({ 0,0,-5 });
    mainCamera->SetPerspective(XM_PIDIV4, float(width) / height, 0.1f, 100.f);

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
    for (auto& gameObject : gameObjects) {
        gameObject->Update(deltaTime);
    }
}

void Renderer::Render() {
    // 현재는 단일스레드 렌더링으로 돌아감

    // 1) 커맨드 기록
    PopulateCommandList();

    // 2) 기록된 커맨드 실행
    ID3D12CommandList* lists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(1, lists);

    // 3) 화면에 표시
    swapChain->Present(1, 0);

    // 4) 다음에 사용할 백버퍼 인덱스 얻기
    frameIndex = swapChain->GetCurrentBackBufferIndex();

    // 5) GPU 완료 대기
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


    // 1. D3D12 디바이스 및 팩토리 생성
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

    // 2. 커맨드 큐 생성
    D3D12_COMMAND_QUEUE_DESC cqDesc = {};
    cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue)));

    // 업로드 전용 커맨드 할당자 & 리스트
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&uploadCommandAllocator)));
    ThrowIfFailed(device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, uploadCommandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&uploadCommandList)));
    uploadCommandList->Close();


    // 3. 스왑체인 생성 (Flip 방식)
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

    // 4. Descriptor Heap 생성 (RTV, DSV)
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

    // 5. 백버퍼용 RTV 생성
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < FrameCount; i++) {
            ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
            device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    // 6. Depth-Stencil 버퍼 생성 및 DSV 생성
    {
        // D3D12_HEAP_PROPERTIES 설정
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

    // 7. 커맨드 할당자 & 커맨드 리스트 생성
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
    commandList->Close();    // 초기엔 닫아두기

    // 8. Fence & 이벤트 생성
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    fenceValue = 1;
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!fenceEvent) return false;

    // 9. Viewport & Scissor 설정
    viewport = { 0.0f, 0.0f, (FLOAT)width, (FLOAT)height, 0.0f, 1.0f };
    scissorRect = { 0, 0, width, height };

    return true;
}


void Renderer::PopulateCommandList() {
    // 1) Reset allocator & list
    commandAllocator->Reset();
    commandList->Reset(commandAllocator.Get(), nullptr);

    // 2) 백버퍼 Transition: PRESENT → RENDER_TARGET
    CD3DX12_RESOURCE_BARRIER barrierToRenderTarget =
        CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrierToRenderTarget);

    // 3) RTV/DSV 바인딩 & 클리어
    auto rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += frameIndex * rtvDescriptorSize;
    auto dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    const FLOAT clearColor[] = { 0,0,0,1 };
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsv,
        D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


    // 4) 뷰포트/시저
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // 5) 각 게임 오브젝트에 그리기 위임
    for (auto& obj : gameObjects) {
        obj->Render(this);
    }

    // 6) 백버퍼 Transition: RENDER_TARGET → PRESENT
    CD3DX12_RESOURCE_BARRIER barrierToPresent =
        CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrierToPresent);

    // 7) 커맨드 리스트 마무리
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
