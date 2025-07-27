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

#include "GameObject.h"
#include "ShaderManager.h"
#include "PipelineStateManager.h"
#include "RootSignatureManager.h"
#include "DescriptorHeapManager.h"
#include "TextureManager.h"
#include "LightingManager.h"
#include "RenderPass/RenderPass.h"
#include "Camera.h"
#include "EnvironmentMaps.h"
#include "ConstantBuffers.h"
#include "ShadowMap.h"


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

using Microsoft::WRL::ComPtr;


inline void ThrowIfFailed(HRESULT hr,
    const char* expr = nullptr,
    const char* file = __FILE__,
    int         line = __LINE__)
{
    if (FAILED(hr))
    {
        auto msg = std::format(
            "ERROR: {} failed at {}:{} (hr=0x{:08X})",
            expr ? expr : "HRESULT",
            file,
            line,
            static_cast<unsigned>(hr));
        throw std::runtime_error(msg);
    }
}

#define THROW_IF_FAILED(x) ThrowIfFailed( \
    (x), #x, __FILE__, __LINE__)

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

    // Direct queue(그래픽스) 접근자
    ID3D12Device* GetDevice() const;
    ID3D12CommandQueue* GetDirectQueue() const;
    ID3D12GraphicsCommandList* GetDirectCommandList() const;
    ID3D12CommandAllocator* GetDirectCommandAllocator() const;
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

    // 카메라
    Camera* GetCamera() const;
    void SetCamera(std::shared_ptr<Camera> cam);

    EnvironmentMaps& GetEnvironmentMaps();
    ID3D12Resource* GetGlobalConstantBuffer() const;


    // Viewport
    int GetViewportWidth() const;
    int GetViewportHeight() const;


    // ImGui 함수들
    bool InitImGui(HWND hwnd);
    void ImGuiNewFrame();
    void ShutdownImGui();


    UINT GetBackBufferIndex() const;

    D3D12_GPU_DESCRIPTOR_HANDLE GetSceneColorSrvHandle() const;
    ID3D12Resource* GetSceneColorBuffer() const;

    const std::array<ShadowMap, MAX_SHADOW_DSV_COUNT>& GetShadowMaps() const;



private:
    bool InitD3D(HWND hwnd, int w, int h);
    void PopulateCommandList();


public:

    static const UINT BackBufferCount = 2;

    enum class RtvIndex : uint8_t {
        SceneColor = 0,
        GBufferAlbedo = 1,
        GBufferNormal = 2,
        GBufferMaterial = 3,
        BackBuffer0 = 4,        // 반드시 마지막에 둘 것
        RtvCount = BackBuffer0 + BackBufferCount
    };

    // 섀도우맵은 라이팅개수만큼 필요하지만 현재구조에선 라이팅 개수는 3개로 고정하였음
    // 추후 경우에 따라 구조변경 필요할 수도 있음
    // Point Light 는 6면 큐브맵이 필요하여 ShadowMap 개수를 정하였음
    enum class DsvIndex : uint8_t {
        DepthStencil = 0,
        ShadowMap0 = 1,
        DsvCount = ShadowMap0 + MAX_SHADOW_DSV_COUNT
    };

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

    // Direct queue & lists
    ComPtr<ID3D12CommandQueue>           directQueue;
    ComPtr<ID3D12CommandAllocator>       directCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>    directCommandList;

    // Copy queue & lists
    ComPtr<ID3D12CommandQueue>           copyQueue;
    ComPtr<ID3D12CommandAllocator>       copyCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>    copyCommandList;

    // Render targets & depth
    ComPtr<ID3D12Resource>          backBuffers[BackBufferCount];
    ComPtr<ID3D12Resource>          GBuffer[UINT(GBufferSlot::Count)];
    ComPtr<ID3D12Resource>          sceneColorBuffer;
    ComPtr<ID3D12Resource>          depthStencilBuffer;

    // Shadow Map
    std::array<ShadowMap, MAX_SHADOW_DSV_COUNT> shadowMaps;

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


    // Global Constant buffer

    CB_Global           globalData;
    ComPtr<ID3D12Resource> globalConstantBuffer;
    UINT8* mappedGlobalPtr = nullptr;           // GPU에 있는 Constant Buffer 리소스를 CPU에서 접근 가능한 메모리 주소로 맵핑된 포인터


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

    // Off‑screen scene render target SRV handle
    DescriptorHandle sceneColorSrvHandle;

};
