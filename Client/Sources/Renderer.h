#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <string>
#include <format>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include "D3DUtil.h"
#include "GameObject.h"
#include "ShaderManager.h"
#include "PipelineStateManager.h"
#include "RootSignatureManager.h"
#include "DescriptorHeapManager.h"
#include "TextureManager.h"
#include "LightingManager.h"
#include "RenderPass/RenderPass.h"
#include "FrameResource/FrameResource.h"
#include "Camera.h"
#include "EnvironmentMaps.h"
#include "ConstantBuffers.h"
#include "ShadowMap.h"
#include "ThreadPool.h"


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

using Microsoft::WRL::ComPtr;


class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize(HWND hwnd, int width, int height);
    void Cleanup();

    // Game object 관리
    void AddGameObject(std::shared_ptr<GameObject> object);
    void RemoveGameObject(std::shared_ptr<GameObject> object);

    const std::vector<std::shared_ptr<GameObject>>& GetOpaqueObjects() const;
    const std::vector<std::shared_ptr<GameObject>>& GetTransparentObjects() const;
    const std::vector<std::shared_ptr<GameObject>>& GetAllGameObjects() const;

    void Update(float deltaTime);
    void Render();

    void UpdateGlobalTime(float seconds);

    bool IsMultithreadedRenderingEnabled() const;


    // Direct queue(그래픽스) 접근자
    ID3D12Device* GetDevice() const;
    ID3D12CommandQueue* GetDirectQueue() const;
    void WaitForDirectQueue();

    // Copy queue(업로드 전용) 접근자
    ID3D12CommandQueue* GetCopyQueue() const;
    ID3D12GraphicsCommandList* GetCopyCommandList() const;
    ID3D12CommandAllocator* GetCopyCommandAllocator() const;
    ID3D12Fence* GetCopyFence() const;
    UINT64& GetCopyFenceValue();
    UINT64 SignalCopyFence();
    void WaitCopyFence(UINT64 value);

    // Manager 접근자
    PipelineStateManager* GetPSOManager() const;
    RootSignatureManager* GetRootSignatureManager() const;
    ShaderManager* GetShaderManager() const;
    DescriptorHeapManager* GetDescriptorHeapManager() const;
    TextureManager* GetTextureManager() const;
    LightingManager* GetLightingManager() const;


    // Multi Thread Commnad 처리
    ThreadPool* GetThreadPool();
    ID3D12CommandAllocator* GetThreadCommandAllocator(UINT index);
    ID3D12GraphicsCommandList* GetThreadCommandList(UINT index);

    void AddRecordedCommandList(ID3D12CommandList* commandList);
    void ClearRecordedCommandLists();
    const std::vector<ID3D12CommandList*>& GetRecordedCommandLists() const;


    // 카메라
    Camera* GetCamera() const;
    void SetCamera(std::shared_ptr<Camera> cam);

    EnvironmentMaps& GetEnvironmentMaps();

    const std::vector<DescriptorHandle>& GetSwapChainRtvs() const;

    // Viewport
    int GetViewportWidth() const;
    int GetViewportHeight() const;


    // ImGui 함수들
    bool InitImGui(HWND hwnd);
    void ImGuiNewFrame();
    void ShutdownImGui();

    UINT GetBackBufferIndex() const;
    FrameResource* GetCurrentFrameResource();

private:
    bool InitD3D(HWND hwnd, int w, int h);

    void RenderSingleThreaded();
    void RecordCommandList_SingleThreaded();

    void RenderMultiThreaded();


public:

    static const UINT BackBufferCount = 3;


    // 섀도우맵은 라이팅개수만큼 필요하지만 현재구조에선 라이팅 개수는 3개로 고정하였음
    // 추후 경우에 따라 구조변경 필요할 수도 있음
    // Point Light 는 6면 큐브맵이 필요하여 ShadowMap 개수를 정하였음


    // 디퍼드렌더링은 현재 미구현
    enum class GBufferSlot : uint8_t {
        Albedo = 0,   // RGB: 알베도, A: Specula
        Normal = 1,   // RGB: 법선, A: Glossiness
        Material = 2,   // R: Metallic, G: Roughness, B: AO, A: 빈 공간/추가 파라미터
        Count = 3
    };


private:

#ifdef _DEBUG
    ComPtr<ID3D12Debug3> debugInterface;
#endif

    // Device & swap chain
    ComPtr<ID3D12Device>            device;
    ComPtr<IDXGISwapChain3>         swapChain;
    UINT                                     backBufferIndex = 0;


    // Back Buffer
    ComPtr<ID3D12Resource>          backBuffers[BackBufferCount];
    std::vector<DescriptorHandle> swapChainRtvs;

    std::unique_ptr<ThreadPool> threadPool;

    bool useMultithreadedRendering = false;

    // Direct queue
    ComPtr<ID3D12CommandQueue>           directQueue;

    // Copy queue & lists
    ComPtr<ID3D12CommandQueue>           copyQueue;
    ComPtr<ID3D12CommandAllocator>       copyCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>    copyCommandList;

    // Thread Pool 용 CommandList & Command Allocator
    std::vector<ComPtr<ID3D12CommandAllocator>> threadCommandAllocators;
    std::vector<ComPtr<ID3D12GraphicsCommandList>> threadCommandLists;
    std::vector<ID3D12CommandList*> recordedCommandLists;

    // Direct-queue Fence
    ComPtr<ID3D12Fence>             directFence;
    UINT64                          directFenceValue = 0;
    HANDLE                          directFenceEvent = nullptr;

    // Copy-queue Fence
    ComPtr<ID3D12Fence>             copyFence;
    UINT64                          copyFenceValue = 0;
    HANDLE                          copyFenceEvent = nullptr;

    // Viewport & Scissor
    D3D12_VIEWPORT                  viewport{};
    D3D12_RECT                      scissorRect{};

    // Managers
    std::unique_ptr<RootSignatureManager>    rootSignatureManager;
    std::unique_ptr<ShaderManager>           shaderManager;
    std::unique_ptr<PipelineStateManager>    psoManager;
    std::unique_ptr<DescriptorHeapManager>   descriptorHeapManager;
    std::unique_ptr<TextureManager>          textureManager;
    std::unique_ptr<LightingManager>         lightingManager;

    std::vector<std::shared_ptr<GameObject>> gameObjects;
    std::vector<std::shared_ptr<GameObject>> opaqueObjects;
    std::vector<std::shared_ptr<GameObject>> transparentObjects;

    std::shared_ptr<Camera>       mainCamera;


    // 환경맵
    EnvironmentMaps environmentMaps;


    enum PassIndex : uint8_t {
        ShadowMap = 0,
        ForwardOpaque,
        ForwardTransparent,
        PostProcess,
        Count
    };

    std::array<std::unique_ptr<RenderPass>, PassIndex::Count> renderPasses;

    std::vector<std::unique_ptr<FrameResource>> frameResources;
    UINT currentFrameIndex = 0;
    FrameResource* currentFrameResource = nullptr;

};
