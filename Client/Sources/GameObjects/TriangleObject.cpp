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
    // 1) ���ؽ�/�ε��� ������ �غ�
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

    // 2) ���ε� Ŀ�ǵ� ����Ʈ Reset
    uploadAllocator->Reset();
    uploadCommandList->Reset(uploadAllocator, nullptr);

    // 3) ���ε� ���� ������ �ø��� (Map �� memcpy)
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

    // 4) ����Ʈ ���� ���ؽ� ���� ����
    {
        CD3DX12_HEAP_PROPERTIES defProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC    bufDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
        device->CreateCommittedResource(
            &defProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&vertexBuffer));
    }

    // 5) ���ؽ� ���� + ���� ��ȯ
    uploadCommandList->CopyResource(vertexBuffer.Get(), vbUpload.Get());
    {
        D3D12_RESOURCE_BARRIER barrier =
            CD3DX12_RESOURCE_BARRIER::Transition(
                vertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        uploadCommandList->ResourceBarrier(1, &barrier);
    }

    // 6) �ε��� ���ε� �� & ���� �غ�
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

    // 7) ����Ʈ ���� �ε��� ���� ����
    {
        CD3DX12_HEAP_PROPERTIES defProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC    bufDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);
        device->CreateCommittedResource(
            &defProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&indexBuffer));
    }

    // 8) �ε��� ���� + ���� ��ȯ
    uploadCommandList->CopyResource(indexBuffer.Get(), ibUpload.Get());
    {
        D3D12_RESOURCE_BARRIER barrier =
            CD3DX12_RESOURCE_BARRIER::Transition(
                indexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_INDEX_BUFFER);
        uploadCommandList->ResourceBarrier(1, &barrier);
    }

    // 9) ���ε� Ŀ�ǵ� ���� & �Ϸ� ���
    uploadCommandList->Close();
    ID3D12CommandList* lists[] = { uploadCommandList };
    queue->ExecuteCommandLists(_countof(lists), lists);
    renderer->WaitForPreviousFrame();

    // 10) GPU �� ����
    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes = sizeof(SimpleVertex);
    vbView.SizeInBytes = vbSize;

    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R32_UINT;
    ibView.SizeInBytes = ibSize;
}

void TriangleObject::Update(float deltaTime)
{
    // �ﰢ�� �̵�
    // Input Manager �� ���� �����¿� �̵� ��Ű��

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

    // 1) PSO & RootSignature ���ε�
    commandList->SetPipelineState(renderer->GetPSOManager()->Get(L"TrianglePSO"));
    commandList->SetGraphicsRootSignature(renderer->GetRootSignatureManager()->Get(L"TriangleRS"));



    // 2) XMMATRIX ���갪�� XMFLOAT4X4�� ����
    CB_MVP cb;

    // worldMatrix�� XMMATRIX Ÿ������ ����� ����� �ִٰ� ����
    XMStoreFloat4x4(&cb.model, XMMatrixTranspose(worldMatrix));

    // ī�޶� �䡤������ ���������� transpose
    XMMATRIX V = renderer->GetCamera()->GetViewMatrix();
    XMMATRIX P = renderer->GetCamera()->GetProjectionMatrix();
    XMStoreFloat4x4(&cb.view, XMMatrixTranspose(V));
    XMStoreFloat4x4(&cb.projection, XMMatrixTranspose(P));

    // ���� ��ȯ�� ����ġ ��Ʈ����
    XMMATRIX invT = XMMatrixInverse(nullptr, worldMatrix);
    XMStoreFloat4x4(&cb.modelInvTranspose, XMMatrixTranspose(invT));

    // memcpy �� MVP ������� ������Ʈ
    memcpy(mappedMVPData, &cb, sizeof(cb));


    // 3) ��� ���� �� ���ε� (b0)
    commandList->SetGraphicsRootConstantBufferView(0, constantMVPBuffer->GetGPUVirtualAddress());

    // 4) IA �¾� & ��ο�
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vbView);
    commandList->IASetIndexBuffer(&ibView);
    commandList->DrawIndexedInstanced((UINT)indices.size(), 1, 0, 0, 0);
}