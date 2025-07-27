#include "Flight.h"
#include "Renderer.h"
#include "Mesh.h"
#include "TextureManager.h"
#include "Material.h"
#include <directx/d3dx12.h>
#include <algorithm>

using namespace DirectX;

Flight::Flight(std::shared_ptr<Mesh> mesh,
    const MaterialPbrTextures& textures)
    : flightMesh(mesh)
{
    materialPBR = std::make_shared<Material>();
    materialPBR->SetAllTextures(textures);

    SetMesh(flightMesh.lock());
}

bool Flight::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
        return false;

    camera = std::make_shared<Camera>();
    camera->SetPosition({ 0.0f, 0.0f, -10.0f });
    float aspect = float(renderer->GetViewportWidth())
        / float(renderer->GetViewportHeight());
    camera->SetPerspective(XM_PIDIV4, aspect, 0.1f, 800.0f);


    ID3D12Device* device = renderer->GetDevice();
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);

    UINT64 rawSize = sizeof(MaterialPbrParameters); 
    UINT64 alignedSize = (rawSize + 255) & ~static_cast<UINT64>(255); // 256�� ����� ������

    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize);

    if (FAILED(device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&materialConstantBuffer))))
        return false;

    materialConstantBuffer->Map(
        0, nullptr,
        reinterpret_cast<void**>(&mappedMaterialBuffer));

    materialPBR->WriteToConstantBuffer(mappedMaterialBuffer);

    // �ؽ�ó ���ҽ� ���� ��ȯ
    auto directList = renderer->GetDirectCommandList();
    auto directAlloc = renderer->GetDirectCommandAllocator();
    auto directQueue = renderer->GetDirectQueue();
    directAlloc->Reset();
    directList->Reset(directAlloc, nullptr);
    for (auto& tex : {
        materialPBR->GetAlbedoTexture(),
        materialPBR->GetNormalTexture(),
        materialPBR->GetMetallicTexture(),
        materialPBR->GetRoughnessTexture() })
    {
        if (!tex) continue;
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            tex->GetResource(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        directList->ResourceBarrier(1, &barrier);
    }
    directList->Close();
    ID3D12CommandList* lists[] = { directList };
    directQueue->ExecuteCommandLists(_countof(lists), lists);
    renderer->WaitForDirectQueue();

    return true;
}

void Flight::Update(float deltaTime)
{
    const float moveSpeed = 3.0f;
    auto& input = InputManager::GetInstance();
    ImGuiIO& io = ImGui::GetIO();
    const float sens = 0.002f;

    // 1) ���콺 �巡��: ����=ī�޶�, ������=������Ʈ
    if (!io.WantCaptureMouse)
    {
        if (input.IsMouseButtonDown(MouseButton::Left))
        {
            /*
            // ī�޶� yaw/pitch
            yawCam += input.GetMouseDeltaX() * sens;
            pitchCam += -input.GetMouseDeltaY() * sens;
            pitchCam = std::clamp(pitchCam, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);

            // ī�޶� ���ʹϾ� ���� & �� ��Ʈ����
            XMVECTOR camQuat = XMQuaternionRotationRollPitchYaw(
                -pitchCam, yawCam, 0.0f);
            camera->SetRotationQuat(camQuat);
            camera->UpdateViewMatrix();
            */
        }
    }

    // 2) Ű �Է����� ī�޶� �̵� (ī�޶� ȸ�� �ݿ�)
    {
        // ���� �� ���� ���
        XMVECTOR camQuat = camera->GetRotationQuat();
        XMVECTOR forward = XMVector3Rotate(
            XMVectorSet(0, 0, 1, 0), camQuat);
        XMVECTOR right = XMVector3Rotate(
            XMVectorSet(1, 0, 0, 0), camQuat);
        XMVECTOR upWorld = XMVectorSet(0, 1, 0, 0); // ���� �� ����

        // ��ġ �����ͼ� �̵�
        /*
        XMFLOAT3 pos = camera->GetPosition();
        XMVECTOR posV = XMLoadFloat3(&pos);
        
        if (input.IsKeyHeld('W')) posV += forward * (moveSpeed * deltaTime);
        if (input.IsKeyHeld('S')) posV -= forward * (moveSpeed * deltaTime);
        if (input.IsKeyHeld('A')) posV -= right * (moveSpeed * deltaTime);
        if (input.IsKeyHeld('D')) posV += right * (moveSpeed * deltaTime);
        if (input.IsKeyHeld('E')) posV -= upWorld * (moveSpeed * deltaTime);
        if (input.IsKeyHeld('Q')) posV += upWorld * (moveSpeed * deltaTime);

        XMStoreFloat3(&pos, posV);
        camera->SetPosition(pos);
        */
    }

    // 3) ImGui: Flight ��Ƽ���� �Ķ����
    if (ImGui::Begin("Flight Material"))
    {
        auto& M = materialPBR->parameters;
        ImGui::ColorEdit3("Base Color",
            reinterpret_cast<float*>(&M.baseColor));
        ImGui::SliderFloat("Metallic", &M.metallic, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular", &M.specular, 0.0f, 10.0f);
        ImGui::SliderFloat("Roughness", &M.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient Occlusion", &M.ambientOcclusion, 0.0f, 10.0f);
        ImGui::ColorEdit3("Emissive Color",
            reinterpret_cast<float*>(&M.emissiveColor));
        ImGui::SliderFloat("Emissive Intensity",
            &M.emissiveIntensity, 0.0f, 10.0f);
    }

    ImGui::End();

    GameObject::Update(deltaTime);
}


void Flight::Render(Renderer* renderer)
{
    // ī�޶� ����
    // renderer->SetCamera(camera);

    // 1) Direct command list ��������
    ID3D12GraphicsCommandList* directCommandList =
        renderer->GetDirectCommandList();

    // 2) Descriptor Heaps ���ε� (CBV_SRV_UAV + SAMPLER)
    ID3D12DescriptorHeap* descriptorHeaps[] = {
        renderer->GetDescriptorHeapManager()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        renderer->GetDescriptorHeapManager()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    directCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);


    // 3) PBR ���� RS & PSO ���ε�
    ID3D12RootSignature* pbrRootSignature = renderer->GetRootSignatureManager()->Get(L"PbrRS");
    ID3D12PipelineState* pbrPso = renderer->GetPSOManager()->Get(L"PbrPSO");

    directCommandList->SetGraphicsRootSignature(pbrRootSignature);
    directCommandList->SetPipelineState(pbrPso);
    
    // 4) ������ ���� CBV ���ε� (b1: Lighting, b3: Global, b4: ShadowMapViewProj)
    directCommandList->SetGraphicsRootConstantBufferView(1, renderer->GetLightingManager()->GetLightingCB()->GetGPUVirtualAddress());
    directCommandList->SetGraphicsRootConstantBufferView(3, renderer->GetGlobalConstantBuffer()->GetGPUVirtualAddress());
    directCommandList->SetGraphicsRootConstantBufferView(4, renderer->GetLightingManager()->GetShadowViewProjCB()->GetGPUVirtualAddress());

    // 5) SRV �� Sampler ���̺� ���ε� (root slot 5~10)
    {
        // material textures (t0~t3) �� root 5
        // �ϳ��� �����ϴ� ������ �ϴ� Root Descriptor Table �������� D3D12_DESCRIPTOR_RANGE �� �Ҵ��߰�
        // 4���� �ؽ��ĸ� ���������� �Ҵ��߱� �����̴�.

        directCommandList->SetGraphicsRootDescriptorTable(
            5, materialPBR->GetAlbedoTexture()->GetGpuHandle());
        
        // IBL ȯ��� ���ε� (t4~t6)
        renderer->GetEnvironmentMaps().Bind(
            directCommandList,
            6,  // Irradiance
            7,  // Specular
            8   // BrdfLut
        );


        // sampler (s0, s1) �� root 9
        // �ϳ��� �����ϴ� ������ �ϴ� Root Descriptor Table �������� D3D12_DESCRIPTOR_RANGE �� �Ҵ��߰�
        // LinearWrapSampler �޸� �Ҵ� �� �ٷ� ClampSampler�� �Ҵ��߱� �����̴�.
        directCommandList->SetGraphicsRootDescriptorTable(
           9, renderer->GetDescriptorHeapManager()->GetLinearWrapSamplerGpuHandle());
        // directCommandList->SetGraphicsRootDescriptorTable(10, renderer->GetDescriptorHeapManager()->GetClampSamplerHandle());
        

        // shadow map SRV ���̺� ���ε� (t7~t7+N-1) �� root 10
        // shadowMaps[i].srvHandle �� �������� �Ҵ�� NUM_SHADOW_DSV_COUNT ���� SRV�� �����Ƿ�,
        // ù �ڵ鸸 �ѱ�� ��ü ���̺��� ���ε���
        const auto& shadowMaps = renderer->GetShadowMaps();
        directCommandList->SetGraphicsRootDescriptorTable(10, shadowMaps[0].srvHandle.gpuHandle);

    }

    // 6) MVP CBV (b0) ������Ʈ & ���ε�
    {
        CB_MVP mvpData{};
        XMStoreFloat4x4(&mvpData.model, XMMatrixTranspose(worldMatrix));
        XMStoreFloat4x4(&mvpData.view, XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix()));
        XMStoreFloat4x4(&mvpData.projection, XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
        XMStoreFloat4x4(&mvpData.modelInvTranspose, XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));
        memcpy(mappedMVPData, &mvpData, sizeof(mvpData));

        directCommandList->SetGraphicsRootConstantBufferView(
            0, constantMVPBuffer->GetGPUVirtualAddress());
    }

    // 7) Material CBV (b2) ������Ʈ & ���ε�
    {
        materialPBR->WriteToConstantBuffer(mappedMaterialBuffer);
        directCommandList->SetGraphicsRootConstantBufferView(
            2, materialConstantBuffer->GetGPUVirtualAddress());
    }

    // 8) IA ���� �� Draw ȣ��
    if (auto mesh = flightMesh.lock())
    {
        directCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        directCommandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        directCommandList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        directCommandList->DrawIndexedInstanced(
            mesh->GetIndexCount(), 1, 0, 0, 0);
    }

}