#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <string>
#include <format>

#include "GameObject.h"
#include "ShaderManager.h"
#include "PipelineStateManager.h"
#include "RootSignatureManager.h"
#include "DescriptorHeapManager.h"
#include "Camera.h"
#include "Light.h"
#include "ConstantBuffers.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

using Microsoft::WRL::ComPtr;

inline void ThrowIfFailed(HRESULT hr, const char* msg = nullptr)
{
    if (FAILED(hr))
    {
        const char* message = (msg && *msg) ? msg : "HRESULT failed";

        throw std::runtime_error(
            std::format("{} (hr=0x{:08X})", message, static_cast<unsigned>(hr))
        );
    }
}

class Renderer {
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

    void WaitForPreviousFrame();

    void UpdateGlobalTime(float timeSeconds);


    ID3D12Device* GetDevice() const;
    ID3D12CommandQueue* GetCommandQueue() const;
    ID3D12GraphicsCommandList* GetCommandList() const;
    ID3D12CommandAllocator* GetCommandAllocator() const;

    // 리소스 생성 시에 사용할 CommandList
    ID3D12CommandAllocator* GetUploadCommandAllocator() const;
    ID3D12GraphicsCommandList* GetUploadCommandList() const;

    PipelineStateManager* GetPSOManager() const;
    RootSignatureManager* GetRootSignatureManager() const;
    ShaderManager* GetShaderManager() const;
    DescriptorHeapManager* GetDescriptorHeapManager() const;


    ID3D12CommandQueue* GetUploadQueue() const;
    ID3D12Fence* GetUploadFence() const;
    UINT64 IncrementUploadFenceValue();


    Camera* GetCamera() const;
    void SetCamera(std::shared_ptr<Camera> newCamera);

    // Viewport 정보
    int GetViewportWidth() const;
    int GetViewportHeight() const;


    ID3D12Resource* GetLightingConstantBuffer()const;
    ID3D12Resource* GetGlobalConstantBuffer()  const;


private:
    bool InitD3D(HWND hwnd, int width, int height);
    void PopulateCommandList();


private:
    static const UINT FrameCount = 2;  

    // Device & swap chain
    ComPtr<ID3D12Device>           device;
    ComPtr<IDXGISwapChain3>        swapChain;
    UINT                            frameIndex = 0;

    // Command queue & lists
    ComPtr<ID3D12CommandQueue>     commandQueue;
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;


    ComPtr<ID3D12CommandAllocator> uploadCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> uploadCommandList;


    // Descriptor heaps: RTV, DSV
    ComPtr<ID3D12DescriptorHeap>   rtvHeap;
    ComPtr<ID3D12DescriptorHeap>   dsvHeap;
    UINT                            rtvDescriptorSize = 0;

    // Render target & depth buffer
    ComPtr<ID3D12Resource>         renderTargets[FrameCount];
    ComPtr<ID3D12Resource>         depthStencilBuffer;

    // Synchronization
    ComPtr<ID3D12Fence>            fence;
    UINT64                          fenceValue = 0;
    HANDLE                          fenceEvent = nullptr;


    // 업로드 전용 동기화 객체
    ComPtr<ID3D12CommandQueue> uploadQueue;
    ComPtr<ID3D12Fence>        uploadFence;
    UINT64                     uploadFenceValue = 0;


    // Viewport & scissor
    D3D12_VIEWPORT                 viewport = {};
    D3D12_RECT                     scissorRect = {};

    // Managers
    std::unique_ptr<RootSignatureManager>   rootSignatureManager;
    std::unique_ptr<ShaderManager>          shaderManager;
    std::unique_ptr<PipelineStateManager>   psoManager;
    std::unique_ptr<DescriptorHeapManager> descriptorHeapManager;

    std::vector<std::shared_ptr<GameObject>> gameObjects;
    std::shared_ptr<Camera>                   mainCamera;

    /* ----------  Frame-global constant buffer : b3  ---------- */
    CB_Global                 globalData;
    ComPtr<ID3D12Resource>    globalConstantBuffer;       // upload heap
    UINT8* mappedGlobalPtr = nullptr;  // persistent map

    /* ----------  Lighting constant buffer : b1  -------------- */
    std::vector<Light>        lights;
    CB_Lighting               lightingData;
    ComPtr<ID3D12Resource>    lightingConstantBuffer;     // upload heap
    UINT8* mappedLightingPtr = nullptr;

    /* ----------  Sampler (s0)  ------------------------------- */
    // RootSignature 에 StaticSampler 로 넣어두면 추가 코드 X
    // 동적 샘플러가 필요하면 ↓ 한 칸만 샘플러 힙에 만들어 공유
    D3D12_GPU_DESCRIPTOR_HANDLE samplerGpuHandle{};



};
