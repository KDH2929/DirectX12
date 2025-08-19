#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <vector>

using Microsoft::WRL::ComPtr;

struct RenderPassCommandBundle
{
    // Pre
    ComPtr<ID3D12CommandAllocator>        preAllocator;
    ComPtr<ID3D12GraphicsCommandList>     preCommandList;

    // 멀티스레드 (N = WorkerThread 수)
    std::vector<ComPtr<ID3D12CommandAllocator>>    threadAllocators;
    std::vector<ComPtr<ID3D12GraphicsCommandList>> threadCommandLists;

    // Post
    ComPtr<ID3D12CommandAllocator>        postAllocator;
    ComPtr<ID3D12GraphicsCommandList>     postCommandList;


    void Initialize(ID3D12Device* device,
        D3D12_COMMAND_LIST_TYPE type,
        UINT numThreads)
    {
        // Pre
        device->CreateCommandAllocator(type, IID_PPV_ARGS(&preAllocator));
        device->CreateCommandList(0, type, preAllocator.Get(), nullptr,
            IID_PPV_ARGS(&preCommandList));

        preCommandList->Close();

        // Threaded
        threadAllocators.resize(numThreads);
        threadCommandLists.resize(numThreads);
        for (UINT i = 0; i < numThreads; ++i) {
            device->CreateCommandAllocator(type, IID_PPV_ARGS(&threadAllocators[i]));
            device->CreateCommandList(0, type, threadAllocators[i].Get(), nullptr,
                IID_PPV_ARGS(&threadCommandLists[i]));

            threadCommandLists[i]->Close();
        }

        // Post
        device->CreateCommandAllocator(type, IID_PPV_ARGS(&postAllocator));
        device->CreateCommandList(0, type, postAllocator.Get(), nullptr,
            IID_PPV_ARGS(&postCommandList));

        postCommandList->Close();
    }

    void ResetAll()
    {
        preAllocator->Reset();
        preCommandList->Reset(preAllocator.Get(), nullptr);

        for (size_t i = 0; i < threadAllocators.size(); ++i) {
            threadAllocators[i]->Reset();
            threadCommandLists[i]->Reset(threadAllocators[i].Get(), nullptr);
        }

        postAllocator->Reset();
        postCommandList->Reset(postAllocator.Get(), nullptr);
    }

    void CloseAll()
    {
        preCommandList->Close();

        for (auto& threadCommandList : threadCommandLists) {
            threadCommandList->Close();
        }

        postCommandList->Close();
    }
};