#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <array>
#include <memory>
#include <barrier>
#include <atomic>
#include <vector>
#include "UploadBuffer.h"
#include "ConstantBuffers.h"
#include "DescriptorHeapManager.h"
#include "ShadowMap.h"
#include "RenderPass/RenderPass.h"
#include "RenderPass/RenderPassCommandBundle.h"

using Microsoft::WRL::ComPtr;

static constexpr UINT MaxShadowMaps = MAX_SHADOW_DSV_COUNT;

class FrameResource
{
public:
    FrameResource(
        ID3D12Device* device,
        DescriptorHeapManager* descriptorHeapManager,
        UINT objectCount,
        UINT frameWidth,
        UINT frameHeight,
        UINT numThreads,
        bool enableMultiThreaded = false);


    // GPU 동기화용 펜스 값
    UINT64 fenceValue = 0;

    // 멀티스레드용
    UINT numThreads = 0;
    bool useMultiThreadedRendering = false;

    std::barrier<> syncPoint;   // Worker Thread 수 + Main Thread 1개


    // 단일 스레드용 커맨드 할당자 & 커맨드 리스트
    ComPtr<ID3D12CommandAllocator>    commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;


    // 멀티 스레드용 커맨드 할당자 & 커맨드 리스트
    ComPtr<ID3D12GraphicsCommandList> preFrameCommandList;
    ComPtr<ID3D12GraphicsCommandList> postFrameCommandList;

    ComPtr<ID3D12CommandAllocator> preFrameAllocator;
    ComPtr<ID3D12CommandAllocator> postFrameAllocator;


    // (N = WorkerThread 수)
    // Shadow pass 용
    RenderPassCommandBundle opaquePassCommandBundle;
    RenderPassCommandBundle shadowPassCommandBundle;


    // 오브젝트당 사용하는 UploadBuffer<CB>
    std::unique_ptr<UploadBuffer<CB_MVP>>           cbMVP;         // objectCount
    std::unique_ptr<UploadBuffer<CB_MaterialPBR>>   cbMaterialPbr; // objectCount
    std::unique_ptr<UploadBuffer<CB_ShadowMapPass>> cbShadowPass;  // objectCount × MaxShadowMaps

    // 단일 슬롯만 필요한 UploadBuffer<CB>
    std::unique_ptr<UploadBuffer<CB_Lighting>>        cbLighting;       // 1
    std::unique_ptr<UploadBuffer<CB_Global>>          cbGlobal;         // 1
    std::unique_ptr<UploadBuffer<CB_OutlineOptions>>  cbOutline;        // 1
    std::unique_ptr<UploadBuffer<CB_ToneMapping>>     cbToneMapping;    // 1
    std::unique_ptr<UploadBuffer<CB_ShadowMapViewProj>> cbShadowViewProj; // 1


    // 렌더 타겟 & 뷰 핸들
    DescriptorHandle sceneColorRtv;    // Off-screen 컬러 RTV
    DescriptorHandle sceneColorSrv;    // Off-screen 컬러 SRV
    DescriptorHandle depthStencilDsv;  // Depth-Stencil DSV

    // ShadowMap용 뷰 핸들
    std::array<DescriptorHandle, MaxShadowMaps> shadowDsv;
    std::array<DescriptorHandle, MaxShadowMaps> shadowSrv;

    // per-object SRV/CBV/UAV 풀 (동적 필요 시)
    std::vector<DescriptorHandle> objectSrvCbvUav;

    // 실제 GPU 리소스
    ComPtr<ID3D12Resource> sceneColorBuffer;
    ComPtr<ID3D12Resource> depthStencilBuffer;
    std::array<ShadowMap, MaxShadowMaps> shadowMaps;


public:
    // 상수 버퍼 초기화
    void InitializeConstantBuffers(
        ID3D12Device* device,
        UINT objectCount);

    // 프레임 단위 Off-screen/Depth/Shadow 리소스 + 뷰 생성
    void InitializeFrameBuffersAndViews(
        ID3D12Device* device,
        DescriptorHeapManager* descriptorHeapManager,
        UINT frameWidth,
        UINT frameHeight);


    // 커맨드 할당자/리스트를 Reset
    void ResetCommandBundles();
    void CloseCommandLists();
    void InitializeCommandBundles(ID3D12Device* device, UINT numThreads);
};
