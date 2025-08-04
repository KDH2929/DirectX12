#include "TriangleObject.h"
#include "Renderer.h"
#include "FrameResource/FrameResource.h"
#include <directx/d3dx12.h>
#include <cassert>

using namespace DirectX;

bool TriangleObject::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
        return false;

    CreateGeometry(renderer);
    return true;
}

void TriangleObject::CreateGeometry(Renderer* renderer)
{
    // 1) Prepare vertex and index data
    vertices = {
        {{ 0.0f,  0.5f, 0.0f }, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f, 0.0f }, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.0f }, {0.0f, 0.0f, 1.0f, 1.0f}},
    };
    indices = { 0, 1, 2 };

    UINT vertexBufferSize = UINT(vertices.size() * sizeof(SimpleVertex));
    UINT indexBufferSize = UINT(indices.size() * sizeof(UINT));

    // 2) Create upload heap and default heap, copy data
    auto* device = renderer->GetDevice();
    auto* copyAlloc = renderer->GetCopyCommandAllocator();
    auto* copyList = renderer->GetCopyCommandList();
    auto* copyQueue = renderer->GetCopyQueue();

    copyAlloc->Reset();
    copyList->Reset(copyAlloc, nullptr);

    // Vertex upload
    {
        ComPtr<ID3D12Resource> upload;
        CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC     desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload));

        UINT8* mapped = nullptr;
        upload->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
        memcpy(mapped, vertices.data(), vertexBufferSize);
        upload->Unmap(0, nullptr);

        CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
        device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&vertexBuffer));

        auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
            vertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        copyList->ResourceBarrier(1, &barrier1);
        copyList->CopyResource(vertexBuffer.Get(), upload.Get());

        auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
            vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
        copyList->ResourceBarrier(1, &barrier2);
    }

    // Index upload
    {
        ComPtr<ID3D12Resource> upload;
        CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC     desc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
        device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload));

        UINT8* mapped = nullptr;
        upload->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
        memcpy(mapped, indices.data(), indexBufferSize);
        upload->Unmap(0, nullptr);

        CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
        device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&indexBuffer));

        auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
            indexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        copyList->ResourceBarrier(1, &barrier1);
        copyList->CopyResource(indexBuffer.Get(), upload.Get());
        auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
            indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
        copyList->ResourceBarrier(1, &barrier2);
    }

    copyList->Close();
    copyQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&copyList));
    UINT64 fenceValue = renderer->SignalCopyFence();
    renderer->WaitCopyFence(fenceValue);

    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();  vertexBufferView.StrideInBytes = sizeof(SimpleVertex);  vertexBufferView.SizeInBytes = vertexBufferSize;
    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();   indexBufferView.Format = DXGI_FORMAT_R32_UINT;           indexBufferView.SizeInBytes = indexBufferSize;
}

void TriangleObject::Update(float deltaTime, Renderer* renderer, UINT objectIndex)
{
    const float moveSpeed = 4.0f;

    auto& input = InputManager::GetInstance();
    if (input.IsKeyHeld(VK_LEFT))  position.x -= moveSpeed * deltaTime;
    if (input.IsKeyHeld(VK_RIGHT)) position.x += moveSpeed * deltaTime;
    if (input.IsKeyHeld(VK_UP))    position.y += moveSpeed * deltaTime;
    if (input.IsKeyHeld(VK_DOWN))  position.y -= moveSpeed * deltaTime;

    GameObject::Update(deltaTime, renderer, objectIndex);
}

void TriangleObject::Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex)
{
    // PSO & RootSignature
    commandList->SetPipelineState(renderer->GetPSOManager()->Get(L"TrianglePSO"));
    commandList->SetGraphicsRootSignature(renderer->GetRootSignatureManager()->Get(L"TriangleRS"));

    // b0: MVP
    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    commandList->SetGraphicsRootConstantBufferView(0, frameResource->cbMVP->GetGPUVirtualAddress(objectIndex));

    // IA & Draw
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);
    commandList->DrawIndexedInstanced(UINT(indices.size()), 1, 0, 0, 0);
}
