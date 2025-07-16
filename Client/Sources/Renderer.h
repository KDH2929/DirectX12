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
#include "Camera.h"
#include "Light.h"
#include "EnvironmentMaps.h"
#include "ConstantBuffers.h"


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

#define CHECK_HR(call) ThrowIfFailed((call), #call, __FILE__, __LINE__)

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Init(HWND hwnd, int width, int height);
    void Cleanup();

    // Game object 관리
    void AddGameObject(std::shared_ptr<GameObject> obj);
    void RemoveGameObject(std::shared_ptr<GameObject> obj);

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

    // 카메라
    Camera* GetCamera() const;
    void SetCamera(std::shared_ptr<Camera> cam);

    EnvironmentMaps& GetEnvironmentMaps();

    // Viewport
    int GetViewportWidth() const;
    int GetViewportHeight() const;

    // 상수 버퍼
    ID3D12Resource* GetLightingConstantBuffer() const;
    ID3D12Resource* GetGlobalConstantBuffer()   const;

    // ImGui 함수들
    bool InitImGui(HWND hwnd);
    void ImGuiNewFrame();
    void ShutdownImGui();

private:
    bool InitD3D(HWND hwnd, int w, int h);
    void PopulateCommandList();

private:
    static const UINT FrameCount = 2;

    // Device & swap chain
    ComPtr<ID3D12Device>            device;
    ComPtr<IDXGISwapChain3>         swapChain;
    UINT                            frameIndex = 0;

    // Direct queue & lists
    ComPtr<ID3D12CommandQueue>           directQueue;
    ComPtr<ID3D12CommandAllocator>       directCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>    directCommandList;

    // Copy queue & lists
    ComPtr<ID3D12CommandQueue>           copyQueue;
    ComPtr<ID3D12CommandAllocator>       copyCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>    copyCommandList;

    // RTV / DSV 힙
    ComPtr<ID3D12DescriptorHeap>    rtvHeap;
    ComPtr<ID3D12DescriptorHeap>    dsvHeap;
    UINT                            rtvDescriptorSize = 0;

    // Render targets & depth
    ComPtr<ID3D12Resource>          renderTargets[FrameCount];
    ComPtr<ID3D12Resource>          depthStencilBuffer;

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

    std::vector<std::shared_ptr<GameObject>> gameObjects;
    std::shared_ptr<Camera>                  mainCamera;

    // Constant buffers
    CB_Global           globalData;
    ComPtr<ID3D12Resource> globalConstantBuffer;
    UINT8* mappedGlobalPtr = nullptr;

    std::vector<Light>  lights;
    CB_Lighting         lightingData;
    ComPtr<ID3D12Resource> lightingConstantBuffer;
    UINT8* mappedLightingPtr = nullptr;


    // 환경맵
    EnvironmentMaps environmentMaps;

};
