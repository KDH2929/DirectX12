#include "TriangleObject.h"
#include "Renderer.h"
#include "InputManager.h"

bool TriangleObject::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
        return false;

    // 지오메트리 생성 및 GPU로 복사
    CreateGeometry(renderer);
    return true;
}

void TriangleObject::CreateGeometry(Renderer* renderer)
{
    // 1) 버텍스/인덱스 데이터 준비
    vertices = {
        {{ 0.0f,  0.5f, 0.0f }, { 1,0,0,1 }},
        {{ 0.5f, -0.5f, 0.0f }, { 0,1,0,1 }},
        {{-0.5f, -0.5f, 0.0f }, { 0,0,1,1 }},
    };
    indices = { 0, 1, 2 };

    const UINT vertexBufferSize = UINT(vertices.size() * sizeof(SimpleVertex));
    const UINT indexBufferSize = UINT(indices.size() * sizeof(UINT));

    // 2) 렌더러로부터 Copy 리소스 가져오기
    auto* device = renderer->GetDevice();
    auto* copyAlloc = renderer->GetCopyCommandAllocator();
    auto* copyList = renderer->GetCopyCommandList();
    auto* copyQueue = renderer->GetCopyQueue();
    auto* copyFence = renderer->GetCopyFence();
    UINT64& copyFenceValue = renderer->GetCopyFenceValue();

    // 3) 커맨드 리스트 준비
    ThrowIfFailed(copyAlloc->Reset());
    ThrowIfFailed(copyList->Reset(copyAlloc, nullptr));

    // 4) 업로드 힙에 버텍스 데이터 올리기
    ComPtr<ID3D12Resource> vertexUpload;
    {
        CD3DX12_HEAP_PROPERTIES heapUP(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC   descUP = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapUP, D3D12_HEAP_FLAG_NONE, &descUP,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&vertexUpload)));

        UINT8* mapped = nullptr;
        vertexUpload->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
        memcpy(mapped, vertices.data(), vertexBufferSize);
        vertexUpload->Unmap(0, nullptr);
    }

    // 5) 디폴트 힙에 버텍스 리소스 생성 (COMMON)
    {
        CD3DX12_HEAP_PROPERTIES heapDef(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC   descVB = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapDef, D3D12_HEAP_FLAG_NONE, &descVB,
            D3D12_RESOURCE_STATE_COMMON, nullptr,
            IID_PPV_ARGS(&this->vertexBuffer)));   // <-- 멤버 사용
    }

    // 6) COMMON ↔ COPY_DEST 복사
    {
        auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
            vertexBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_DEST);
        copyList->ResourceBarrier(1, &barrier1);

        copyList->CopyResource(vertexBuffer.Get(), vertexUpload.Get());

        auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
            vertexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COMMON);
        copyList->ResourceBarrier(1, &barrier2);
    }

    // 7) 업로드 힙에 인덱스 데이터 올리기
    ComPtr<ID3D12Resource> indexUpload;
    {
        CD3DX12_HEAP_PROPERTIES heapUP(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC   descUP = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapUP, D3D12_HEAP_FLAG_NONE, &descUP,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&indexUpload)));

        UINT8* mapped = nullptr;
        indexUpload->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
        memcpy(mapped, indices.data(), indexBufferSize);
        indexUpload->Unmap(0, nullptr);
    }

    // 8) 디폴트 힙에 인덱스 리소스 생성 (COMMON)
    {
        CD3DX12_HEAP_PROPERTIES heapDef(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC   descIB = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapDef, D3D12_HEAP_FLAG_NONE, &descIB,
            D3D12_RESOURCE_STATE_COMMON, nullptr,
            IID_PPV_ARGS(&this->indexBuffer)));  // <-- 멤버 사용
    }

    // 9) COMMON ↔ COPY_DEST 복사
    {
        auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
            indexBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_DEST);
        copyList->ResourceBarrier(1, &barrier1);

        copyList->CopyResource(indexBuffer.Get(), indexUpload.Get());

        auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
            indexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COMMON);
        copyList->ResourceBarrier(1, &barrier2);
    }

    // 10) 복사 실행 및 대기
    copyList->Close();
    ID3D12CommandList* lists[] = { copyList };
    copyQueue->ExecuteCommandLists(_countof(lists), lists);

    const UINT64 fenceValue = renderer->SignalCopyFence();
    renderer->WaitCopyFence(fenceValue);

    // 11) VB/IB 뷰 설정
    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes = sizeof(SimpleVertex);
    vbView.SizeInBytes = vertexBufferSize;

    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R32_UINT;
    ibView.SizeInBytes = indexBufferSize;
}


void TriangleObject::Update(float deltaTime)
{
    // 화살표 키로 삼각형 이동
    if (InputManager::GetInstance().IsKeyHeld(VK_LEFT))
        position.x -= 1.0f * deltaTime;
    if (InputManager::GetInstance().IsKeyHeld(VK_RIGHT))
        position.x += 1.0f * deltaTime;
    if (InputManager::GetInstance().IsKeyHeld(VK_UP))
        position.y += 1.0f * deltaTime;
    if (InputManager::GetInstance().IsKeyHeld(VK_DOWN))
        position.y -= 1.0f * deltaTime;

    GameObject::Update(deltaTime);
}

void TriangleObject::Render(Renderer* renderer)
{
    ID3D12GraphicsCommandList* commandList = renderer->GetDirectCommandList();

    // 1) PSO 및 RootSignature 바인딩
    commandList->SetPipelineState(renderer->GetPSOManager()->Get(L"TrianglePSO"));
    commandList->SetGraphicsRootSignature(renderer->GetRootSignatureManager()->Get(L"TriangleRS"));

    // 2) 상수버퍼에 MVP 값 복사
    CB_MVP cbData;
    XMStoreFloat4x4(&cbData.model, XMMatrixTranspose(worldMatrix));
    XMStoreFloat4x4(&cbData.view, XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix()));
    XMStoreFloat4x4(&cbData.projection, XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
    XMStoreFloat4x4(&cbData.modelInvTranspose, XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));

    memcpy(mappedMVPData, &cbData, sizeof(cbData));

    // 3) 상수버퍼 뷰 바인딩 (b0)
    commandList->SetGraphicsRootConstantBufferView(0, constantMVPBuffer->GetGPUVirtualAddress());

    // 4) IA 셋업 및 드로우
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vbView);
    commandList->IASetIndexBuffer(&ibView);
    commandList->DrawIndexedInstanced((UINT)indices.size(), 1, 0, 0, 0);
}
