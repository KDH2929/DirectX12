#include "TriangleObject.h"
#include "Renderer.h"

bool TriangleObject::Initialize(Renderer* renderer) {
    if (!GameObject::Initialize(renderer))
        return false;

    // Build geometry and upload to GPU
    CreateGeometry(renderer);
    return true;
}

void TriangleObject::CreateGeometry(Renderer* renderer) {
    // 1) 버텍스/인덱스 데이터 준비
    vertices = {
        {{ 0.0f,  0.5f, 0.0f}, {1,0,0,1}},
        {{ 0.5f, -0.5f, 0.0f}, {0,1,0,1}},
        {{-0.5f, -0.5f, 0.0f}, {0,0,1,1}},
    };
    indices = { 0,1,2 };

    const UINT vbSize = UINT(vertices.size() * sizeof(SimpleVertex));
    const UINT ibSize = UINT(indices.size() * sizeof(UINT));

    ID3D12Device* device = renderer->GetDevice();
    auto queue = renderer->GetCommandQueue();
    auto uploadAllocator = renderer->GetUploadCommandAllocator();
    auto uploadCommandList = renderer->GetUploadCommandList();

    // 2) 업로드 커맨드 리스트 Reset
    uploadAllocator->Reset();
    uploadCommandList->Reset(uploadAllocator, nullptr);

    // 3) 업로드 힙에 데이터 올리기 (Map → memcpy)
    ComPtr<ID3D12Resource> vbUpload;
    {
        CD3DX12_HEAP_PROPERTIES upProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC    bufDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
        device->CreateCommittedResource(
            &upProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&vbUpload));

        UINT8* pData;
        vbUpload->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        memcpy(pData, vertices.data(), vbSize);
        vbUpload->Unmap(0, nullptr);
    }

    // 4) 디폴트 힙에 버텍스 버퍼 생성
    {
        CD3DX12_HEAP_PROPERTIES defProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC    bufDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
        device->CreateCommittedResource(
            &defProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&vertexBuffer));
    }

    // 5) 버텍스 복사 + 상태 전환
    uploadCommandList->CopyResource(vertexBuffer.Get(), vbUpload.Get());
    {
        D3D12_RESOURCE_BARRIER barrier =
            CD3DX12_RESOURCE_BARRIER::Transition(
                vertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        uploadCommandList->ResourceBarrier(1, &barrier);
    }

    // 6) 인덱스 업로드 힙 & 복사 준비
    ComPtr<ID3D12Resource> ibUpload;
    {
        CD3DX12_HEAP_PROPERTIES upProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC    bufDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);
        device->CreateCommittedResource(
            &upProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&ibUpload));

        UINT8* pData;
        ibUpload->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        memcpy(pData, indices.data(), ibSize);
        ibUpload->Unmap(0, nullptr);
    }

    // 7) 디폴트 힙에 인덱스 버퍼 생성
    {
        CD3DX12_HEAP_PROPERTIES defProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC    bufDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);
        device->CreateCommittedResource(
            &defProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&indexBuffer));
    }

    // 8) 인덱스 복사 + 상태 전환
    uploadCommandList->CopyResource(indexBuffer.Get(), ibUpload.Get());
    {
        D3D12_RESOURCE_BARRIER barrier =
            CD3DX12_RESOURCE_BARRIER::Transition(
                indexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_INDEX_BUFFER);
        uploadCommandList->ResourceBarrier(1, &barrier);
    }

    // 9) 업로드 커맨드 실행 & 완료 대기
    uploadCommandList->Close();
    ID3D12CommandList* lists[] = { uploadCommandList };
    queue->ExecuteCommandLists(_countof(lists), lists);
    renderer->WaitForPreviousFrame();

    // 10) GPU 뷰 설정
    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes = sizeof(SimpleVertex);
    vbView.SizeInBytes = vbSize;

    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R32_UINT;
    ibView.SizeInBytes = ibSize;
}

void TriangleObject::Update(float deltaTime)
{
    // 삼각형 이동
    // Input Manager 를 통해 상하좌우 이동 시키기

    if (InputManager::GetInstance().IsKeyHeld(VK_LEFT)) {
        position.x -= 1.0f * deltaTime;
    }
    if (InputManager::GetInstance().IsKeyHeld(VK_RIGHT)) {
        position.x += 1.0f * deltaTime;
    }

    if (InputManager::GetInstance().IsKeyHeld(VK_UP)) {
        position.y += 1.0f * deltaTime;
    }
    if (InputManager::GetInstance().IsKeyHeld(VK_DOWN)) {
        position.y -= 1.0f * deltaTime;
    }

    GameObject::Update(deltaTime);
}

void TriangleObject::Render(Renderer* renderer) {
    auto commandList = renderer->GetCommandList();

    // 1) PSO & RootSignature 바인딩
    commandList->SetPipelineState(renderer->GetPSOManager()->Get(L"TrianglePSO"));
    commandList->SetGraphicsRootSignature(renderer->GetRootSignatureManager()->Get(L"TriangleRS"));



    // 2) XMMATRIX 연산값을 XMFLOAT4X4에 저장
    CB_MVP cb;

    // worldMatrix는 XMMATRIX 타입으로 멤버에 저장돼 있다고 가정
    XMStoreFloat4x4(&cb.model, XMMatrixTranspose(worldMatrix));

    // 카메라 뷰·투영도 마찬가지로 transpose
    XMMATRIX V = renderer->GetCamera()->GetViewMatrix();
    XMMATRIX P = renderer->GetCamera()->GetProjectionMatrix();
    XMStoreFloat4x4(&cb.view, XMMatrixTranspose(V));
    XMStoreFloat4x4(&cb.projection, XMMatrixTranspose(P));

    // 법선 변환용 역전치 매트릭스
    XMMATRIX invT = XMMatrixInverse(nullptr, worldMatrix);
    XMStoreFloat4x4(&cb.modelInvTranspose, XMMatrixTranspose(invT));

    // memcpy 로 MVP 상수버퍼 업데이트
    memcpy(mappedMVPData, &cb, sizeof(cb));


    // 3) 상수 버퍼 뷰 바인딩 (b0)
    commandList->SetGraphicsRootConstantBufferView(0, constantMVPBuffer->GetGPUVirtualAddress());

    // 4) IA 셋업 & 드로우
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vbView);
    commandList->IASetIndexBuffer(&ibView);
    commandList->DrawIndexedInstanced((UINT)indices.size(), 1, 0, 0, 0);
}