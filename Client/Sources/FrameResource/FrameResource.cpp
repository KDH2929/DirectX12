#include "FrameResource.h"
#include "D3DUtil.h" 
#include <dxgi1_6.h>
#include <directx/d3dx12.h>

FrameResource::FrameResource(
    ID3D12Device* device,
    DescriptorHeapManager* descriptorHeapManager,
    UINT objectCount,
    UINT frameWidth,
    UINT frameHeight,
    UINT numThreads,
    bool enableMultiThreaded)
    : numThreads(numThreads)
    ,useMultiThreadedRendering(enableMultiThreaded)
    ,syncPoint(numThreads+1)
{

    InitializeCommandBundles(device, numThreads);
    

    // 상수 버퍼들 초기화
    InitializeConstantBuffers(device, objectCount);

    // Off-screen, Depth, ShadowMap 리소스 + 뷰 생성
    InitializeFrameBuffersAndViews(
        device,
        descriptorHeapManager,
        frameWidth,
        frameHeight);
}

void FrameResource::InitializeConstantBuffers(
    ID3D12Device* device,
    UINT objectCount)
{
    cbMVP = std::make_unique<UploadBuffer<CB_MVP>>(device, objectCount, true);
    cbMaterialPbr = std::make_unique<UploadBuffer<CB_MaterialPBR>>(device, objectCount, true);
    cbShadowPass = std::make_unique<UploadBuffer<CB_ShadowMapPass>>(device, objectCount * MaxShadowMaps, true);

    cbLighting = std::make_unique<UploadBuffer<CB_Lighting>>(device, 1, true);
    cbGlobal = std::make_unique<UploadBuffer<CB_Global>>(device, 1, true);
    cbOutline = std::make_unique<UploadBuffer<CB_OutlineOptions>>(device, 1, true);
    cbToneMapping = std::make_unique<UploadBuffer<CB_ToneMapping>>(device, 1, true);
    cbShadowViewProj = std::make_unique<UploadBuffer<CB_ShadowMapViewProj>>(device, 1, true);
}

void FrameResource::InitializeFrameBuffersAndViews(
    ID3D12Device* device,
    DescriptorHeapManager* descriptorHeapManager,
    UINT frameWidth,
    UINT frameHeight)
{

    // 1) Off-screen SceneColor 버퍼 생성 + RTV/SRV 뷰
    {
        // 텍스처 설명자
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Width = frameWidth;
        textureDesc.Height = frameHeight;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        textureDesc.SampleDesc.Count = 1;

        // 클리어 값
        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = textureDesc.Format;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;

        // 힙 속성 (DEFAULT)
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

        // 리소스 생성
        THROW_IF_FAILED(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValue,
            IID_PPV_ARGS(&sceneColorBuffer)));

        // RTV 생성
        sceneColorRtv = descriptorHeapManager->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        descriptorHeapManager->CreateRenderTargetView(
            device,
            sceneColorBuffer.Get(),
            sceneColorRtv.index);

        // SRV 생성
        sceneColorSrv = descriptorHeapManager->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        device->CreateShaderResourceView(
            sceneColorBuffer.Get(),
            nullptr,
            sceneColorSrv.cpuHandle);
    }

    // 2) Depth-Stencil 버퍼 생성 + DSV 뷰
    {
        D3D12_RESOURCE_DESC depthDesc = {};
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Width = frameWidth;
        depthDesc.Height = frameHeight;
        depthDesc.DepthOrArraySize = 1;
        depthDesc.MipLevels = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        depthDesc.SampleDesc.Count = 1;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = depthDesc.Format;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

        THROW_IF_FAILED(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&depthStencilBuffer)));

        depthStencilDsv = descriptorHeapManager->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        descriptorHeapManager->CreateDepthStencilView(
            device,
            depthStencilBuffer.Get(),
            nullptr,
            depthStencilDsv.index);
    }

    // 3) ShadowMap 버퍼들 생성 + DSV/SRV 뷰
    for (UINT i = 0; i < MaxShadowMaps; ++i)
    {
        D3D12_RESOURCE_DESC shadowDesc = {};
        shadowDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        shadowDesc.Width = SHADOW_MAP_WIDTH;
        shadowDesc.Height = SHADOW_MAP_HEIGHT;
        shadowDesc.DepthOrArraySize = 1;
        shadowDesc.MipLevels = 1;
        shadowDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
        shadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        shadowDesc.SampleDesc.Count = 1;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

        // 깊이-스텐실 버퍼 생성
        THROW_IF_FAILED(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &shadowDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&shadowMaps[i].depthBuffer)));

        // DSV 뷰
        shadowDsv[i] = descriptorHeapManager->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        descriptorHeapManager->CreateDepthStencilView(
            device,
            shadowMaps[i].depthBuffer.Get(),
            &dsvDesc,
            shadowDsv[i].index);

        // SRV 뷰
        shadowSrv[i] = descriptorHeapManager->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        device->CreateShaderResourceView(
            shadowMaps[i].depthBuffer.Get(),
            &srvDesc,
            shadowSrv[i].cpuHandle);
    }

    // 4) per-object SRV/CBV/UAV 풀 초기화
    // 미사용
    objectSrvCbvUav.clear();
}

void FrameResource::InitializeCommandBundles(ID3D12Device* device, UINT numThreads)
{
    // 단일 스레드용 커맨드 할당자 & 리스트 생성
    THROW_IF_FAILED(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator)));
    THROW_IF_FAILED(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&commandList)));
    commandList->Close();

    if (useMultiThreadedRendering) {
        // PreFrame
        THROW_IF_FAILED(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&preFrameAllocator)));
        THROW_IF_FAILED(device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            preFrameAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&preFrameCommandList)));
        preFrameCommandList->Close();

        // PostFrame
        THROW_IF_FAILED(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&postFrameAllocator)));
        THROW_IF_FAILED(device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            postFrameAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&postFrameCommandList)));
        postFrameCommandList->Close();


        // Pass Command Bundle
        shadowPassCommandBundle.Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT, numThreads);
        opaquePassCommandBundle.Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT, numThreads);
        
    }
}

void FrameResource::ResetCommandBundles()
{
    commandAllocator->Reset();
    commandList->Reset(commandAllocator.Get(), nullptr);

    if (useMultiThreadedRendering) {
        // preRender
        preFrameAllocator->Reset();
        preFrameCommandList->Reset(preFrameAllocator.Get(), nullptr);

        // postRender
        postFrameAllocator->Reset();
        postFrameCommandList->Reset(postFrameAllocator.Get(), nullptr);

        shadowPassCommandBundle.ResetAll();
        opaquePassCommandBundle.ResetAll();
    }
}

void FrameResource::CloseCommandLists()
{
    commandList->Close();
}
