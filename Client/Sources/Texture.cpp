#include "Texture.h"
#include "Renderer.h"
#include <DirectXTex.h>
#include <codecvt>
#include <directx/d3dx12.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// 파일을 로드하여 GPU 텍스처 생성 후 COMMON 상태로 전환
// (Copy 명령 Close → Execute → Fence Wait를 내부에서 처리)

bool Texture::LoadFromFile(Renderer* renderer, const std::wstring& filePath, bool generateMips)
{
    name = filePath;

    // 렌더러에서 Copy 전용 커맨드 리스트/할당자/큐/펜스 정보 가져오기
    ID3D12Device* device = renderer->GetDevice();
    auto         copyList = renderer->GetCopyCommandList();
    auto         copyAllocator = renderer->GetCopyCommandAllocator();
    auto         copyQueue = renderer->GetCopyQueue();
    auto         copyFence = renderer->GetCopyFence();
    UINT64& copyFenceValue = renderer->GetCopyFenceValue();

    // 1) 이미지 로드 (DDS / WIC)
    ScratchImage scratchImage;
    TexMetadata  metadata;
    UINT         arraySize = 1;
    UINT         mipLevels = 1;

    std::wstring ext = filePath.substr(filePath.find_last_of(L'.'));
    if (_wcsicmp(ext.c_str(), L".dds") == 0)
    {
        // DDS는 이미 MIP 레벨 포함
        if (FAILED(LoadFromDDSFile(filePath.c_str(), DDS_FLAGS_NONE, &metadata, scratchImage)))
            return false;

        arraySize = static_cast<UINT>(metadata.arraySize);
        mipLevels = static_cast<UINT>(metadata.mipLevels);
    }
    else
    {
        // WIC 로드: 원본 레벨만
        if (FAILED(LoadFromWICFile(filePath.c_str(), WIC_FLAGS_FORCE_RGB, &metadata, scratchImage)))
            return false;

        // 밉맵 체인 생성
        {
            ScratchImage mipChain;
            HRESULT hr = GenerateMipMaps(
                scratchImage.GetImages(),
                scratchImage.GetImageCount(),
                scratchImage.GetMetadata(),
                TEX_FILTER_DEFAULT,
                0,            // 0이면 최대 레벨까지
                mipChain
            );
            if (FAILED(hr))
                return false;

            scratchImage = std::move(mipChain);
            metadata = scratchImage.GetMetadata();
        }

        arraySize = 1;
        mipLevels = static_cast<UINT>(metadata.mipLevels);
    }

    // 2) GPU 리소스 생성 (Default heap, COMMON 상태)
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC   desc = CD3DX12_RESOURCE_DESC::Tex2D(
        metadata.format,
        static_cast<UINT>(metadata.width),
        static_cast<UINT>(metadata.height),
        arraySize,
        mipLevels);

    if (FAILED(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&texture))))
        return false;

    // 3) Upload heap (staging) 버퍼 생성
    UINT64 uploadSize = GetRequiredIntermediateSize(texture.Get(), 0, arraySize * mipLevels);
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

    ComPtr<ID3D12Resource> uploadResource;
    if (FAILED(device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadResource))))
        return false;

    // 4) Copy 리스트 준비: COMMON -> COPY_DEST
    copyAllocator->Reset();
    copyList->Reset(copyAllocator, nullptr);
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            texture.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_DEST);
        copyList->ResourceBarrier(1, &barrier);
    }

    // 5) 서브리소스 데이터 준비 & 복사
    UINT                   subCount = arraySize * mipLevels;
    std::vector<D3D12_SUBRESOURCE_DATA> subresources(subCount);
    auto images = scratchImage.GetImages();
    for (UINT i = 0; i < subCount; ++i)
    {
        subresources[i] = {
            images[i].pixels,
            static_cast<LONG_PTR>(images[i].rowPitch),
            static_cast<LONG_PTR>(images[i].slicePitch)
        };
    }
    UpdateSubresources(
        copyList,
        texture.Get(),
        uploadResource.Get(),
        0, 0,
        subCount,
        subresources.data());

    // 6) COPY_DEST -> COMMON
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            texture.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COMMON);
        copyList->ResourceBarrier(1, &barrier);
    }
    copyList->Close();

    // 7) Copy 큐 제출 및 동기화
    ID3D12CommandList* lists[] = { copyList };
    copyQueue->ExecuteCommandLists(_countof(lists), lists);
    ++copyFenceValue;
    copyQueue->Signal(copyFence, copyFenceValue);
    if (copyFence->GetCompletedValue() < copyFenceValue)
    {
        HANDLE evt = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        copyFence->SetEventOnCompletion(copyFenceValue, evt);
        WaitForSingleObject(evt, INFINITE);
        CloseHandle(evt);
    }

    return true;
}

bool Texture::LoadCubeMapFromFile(Renderer* renderer, const std::wstring& filePath, bool generateMips)
{
    name = filePath;

    // 복사용 커맨드 정보
    ID3D12Device* device = renderer->GetDevice();
    ID3D12GraphicsCommandList* copyList = renderer->GetCopyCommandList();
    ID3D12CommandAllocator* copyAllocator = renderer->GetCopyCommandAllocator();
    ID3D12CommandQueue* copyQueue = renderer->GetCopyQueue();
    ID3D12Fence* copyFence = renderer->GetCopyFence();
    UINT64& fenceValue = renderer->GetCopyFenceValue();

    // 1) DDS 큐브맵 로드
    ScratchImage scratchImage;
    TexMetadata metadata;
    if (FAILED(LoadFromDDSFile(filePath.c_str(), DDS_FLAGS_NONE, &metadata, scratchImage)))
    {
        return false;
    }

    if (generateMips && metadata.mipLevels == 1)
    {
        ScratchImage mipChain;
        HRESULT hr = GenerateMipMaps(
            scratchImage.GetImages(),
            scratchImage.GetImageCount(),
            scratchImage.GetMetadata(),
            TEX_FILTER_DEFAULT,
            0,              // 0 = 자동으로 최대 레벨까지
            mipChain
        );
        if (FAILED(hr))
            return false;
        scratchImage = std::move(mipChain);
        metadata = scratchImage.GetMetadata();
    }

    // 2) 리소스 설명 구성 (6 faces)
    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = static_cast<UINT>(metadata.width);
    textureDesc.Height = static_cast<UINT>(metadata.height);
    textureDesc.DepthOrArraySize = static_cast<UINT>(metadata.arraySize);  // 6 faces
    textureDesc.MipLevels = static_cast<UINT>(metadata.mipLevels);
    textureDesc.Format = metadata.format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // 3) GPU 리소스 생성 (초기 상태 = COMMON)
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    if (FAILED(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&texture))))
    {
        return false;
    }

    // 4) staging 버퍼 및 서브리소스 개수 계산
    UINT   subresourceCount = textureDesc.MipLevels * textureDesc.DepthOrArraySize;
    UINT64 requiredSize = GetRequiredIntermediateSize(texture.Get(), 0, subresourceCount);

    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    auto stagingDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);

    ComPtr<ID3D12Resource> uploadBuffer;
    if (FAILED(device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &stagingDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer))))
    {
        return false;
    }

    // 5) 서브리소스 데이터 준비
    auto images = scratchImage.GetImages();
    size_t imageCount = scratchImage.GetImageCount();

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    subresources.reserve(imageCount);
    for (size_t i = 0; i < imageCount; ++i)
    {
        D3D12_SUBRESOURCE_DATA data{};
        data.pData = images[i].pixels;
        data.RowPitch = images[i].rowPitch;
        data.SlicePitch = images[i].slicePitch;
        subresources.push_back(data);
    }

    // 6) copy 커맨드 기록
    copyAllocator->Reset();
    copyList->Reset(copyAllocator, nullptr);

    // 6-1) COMMON -> COPY_DEST
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            texture.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_DEST);
        copyList->ResourceBarrier(1, &barrier);
    }

    // 6-2) 실제 복사
    UpdateSubresources(
        copyList,
        texture.Get(),
        uploadBuffer.Get(),
        0, 0,
        subresourceCount,
        subresources.data());

    // 6-3) COPY_DEST -> COMMON
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            texture.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COMMON);
        copyList->ResourceBarrier(1, &barrier);
    }

    copyList->Close();
    ID3D12CommandList* lists[] = { copyList };
    copyQueue->ExecuteCommandLists(_countof(lists), lists);

    // 7) 동기화
    ++fenceValue;
    copyQueue->Signal(copyFence, fenceValue);
    if (copyFence->GetCompletedValue() < fenceValue)
    {
        HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        copyFence->SetEventOnCompletion(fenceValue, event);
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }

    return true;
}

ID3D12Resource* Texture::GetResource() const { return texture.Get(); }
D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetGpuHandle() const { return gpuHandle; }
D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetCpuHandle() const { return cpuHandle; }
UINT Texture::GetDescriptorIndex() const { return descriptorIndex; }
const std::wstring& Texture::GetName() const { return name; }

void Texture::SetDescriptorHandles(
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle_,
    UINT index_)
{
    cpuHandle = cpuHandle_;
    gpuHandle = gpuHandle_;
    descriptorIndex = index_;
}