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


    // GPU ����ȭ�� �潺 ��
    UINT64 fenceValue = 0;

    // ��Ƽ�������
    UINT numThreads = 0;
    bool useMultiThreadedRendering = false;

    std::barrier<> syncPoint;   // Worker Thread �� + Main Thread 1��


    // ���� ������� Ŀ�ǵ� �Ҵ��� & Ŀ�ǵ� ����Ʈ
    ComPtr<ID3D12CommandAllocator>    commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;


    // ��Ƽ ������� Ŀ�ǵ� �Ҵ��� & Ŀ�ǵ� ����Ʈ
    ComPtr<ID3D12GraphicsCommandList> preFrameCommandList;
    ComPtr<ID3D12GraphicsCommandList> postFrameCommandList;

    ComPtr<ID3D12CommandAllocator> preFrameAllocator;
    ComPtr<ID3D12CommandAllocator> postFrameAllocator;


    // (N = WorkerThread ��)
    // Shadow pass ��
    RenderPassCommandBundle opaquePassCommandBundle;
    RenderPassCommandBundle shadowPassCommandBundle;


    // ������Ʈ�� ����ϴ� UploadBuffer<CB>
    std::unique_ptr<UploadBuffer<CB_MVP>>           cbMVP;         // objectCount
    std::unique_ptr<UploadBuffer<CB_MaterialPBR>>   cbMaterialPbr; // objectCount
    std::unique_ptr<UploadBuffer<CB_ShadowMapPass>> cbShadowPass;  // objectCount �� MaxShadowMaps

    // ���� ���Ը� �ʿ��� UploadBuffer<CB>
    std::unique_ptr<UploadBuffer<CB_Lighting>>        cbLighting;       // 1
    std::unique_ptr<UploadBuffer<CB_Global>>          cbGlobal;         // 1
    std::unique_ptr<UploadBuffer<CB_OutlineOptions>>  cbOutline;        // 1
    std::unique_ptr<UploadBuffer<CB_ToneMapping>>     cbToneMapping;    // 1
    std::unique_ptr<UploadBuffer<CB_ShadowMapViewProj>> cbShadowViewProj; // 1


    // ���� Ÿ�� & �� �ڵ�
    DescriptorHandle sceneColorRtv;    // Off-screen �÷� RTV
    DescriptorHandle sceneColorSrv;    // Off-screen �÷� SRV
    DescriptorHandle depthStencilDsv;  // Depth-Stencil DSV

    // ShadowMap�� �� �ڵ�
    std::array<DescriptorHandle, MaxShadowMaps> shadowDsv;
    std::array<DescriptorHandle, MaxShadowMaps> shadowSrv;

    // per-object SRV/CBV/UAV Ǯ (���� �ʿ� ��)
    std::vector<DescriptorHandle> objectSrvCbvUav;

    // ���� GPU ���ҽ�
    ComPtr<ID3D12Resource> sceneColorBuffer;
    ComPtr<ID3D12Resource> depthStencilBuffer;
    std::array<ShadowMap, MaxShadowMaps> shadowMaps;


public:
    // ��� ���� �ʱ�ȭ
    void InitializeConstantBuffers(
        ID3D12Device* device,
        UINT objectCount);

    // ������ ���� Off-screen/Depth/Shadow ���ҽ� + �� ����
    void InitializeFrameBuffersAndViews(
        ID3D12Device* device,
        DescriptorHeapManager* descriptorHeapManager,
        UINT frameWidth,
        UINT frameHeight);


    // Ŀ�ǵ� �Ҵ���/����Ʈ�� Reset
    void ResetCommandBundles();
    void CloseCommandLists();
    void InitializeCommandBundles(ID3D12Device* device, UINT numThreads);
};
