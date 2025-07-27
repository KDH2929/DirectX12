#include "SphereObject.h"
#include "Renderer.h"
#include "Mesh.h"
#include <directx/d3dx12.h>
#include <imgui.h>
#include <algorithm>

using namespace DirectX;

SphereObject::SphereObject(
    std::shared_ptr<Material> material,
    uint32_t latitudeSegments,
    uint32_t longitudeSegments)
    : latitudeSegments(latitudeSegments),
    longitudeSegments(longitudeSegments),
    materialPBR(std::move(material))
{
}

SphereObject::~SphereObject()
{
    if (materialConstantBuffer && mappedMaterialBuffer) {
        materialConstantBuffer->Unmap(0, nullptr);
    }
}

bool SphereObject::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer)) {
        return false;
    }

    camera = std::make_shared<Camera>();
    camera->SetPosition({ 0, 0, -10 });
    float aspect = float(renderer->GetViewportWidth()) / float(renderer->GetViewportHeight());
    camera->SetPerspective(XM_PIDIV4, aspect, 0.1f, 1000.f);

    // 1) �� �޽� ����
    sphereMesh = Mesh::CreateSphere(renderer, latitudeSegments, longitudeSegments);
    if (!sphereMesh) {
        return false;
    }

    SetMesh(sphereMesh);

    // 2) ��Ƽ���� ��� ���� ���� (256B aligned)
    auto device = renderer->GetDevice();
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(256);

    if (FAILED(device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&materialConstantBuffer))))
    {
        return false;
    }

    materialConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedMaterialBuffer));
    materialPBR->WriteToConstantBuffer(mappedMaterialBuffer);

    // 3) �ؽ�ó ���ҽ� ���� ��ȯ
    auto directAlloc = renderer->GetDirectCommandAllocator();
    auto directList = renderer->GetDirectCommandList();
    auto directQueue = renderer->GetDirectQueue();

    directAlloc->Reset();
    directList->Reset(directAlloc, nullptr);

    for (auto& texPtr : {
        materialPBR->GetAlbedoTexture(),
        materialPBR->GetNormalTexture(),
        materialPBR->GetMetallicTexture(),
        materialPBR->GetRoughnessTexture() })
    {
        if (!texPtr) {
            continue;
        }
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            texPtr->GetResource(),
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

void SphereObject::Update(float deltaTime)
{
    auto& input = InputManager::GetInstance();
    ImGuiIO& io = ImGui::GetIO();
    const float sens = 0.002f;

    // 1) ���콺 �巡��: ����=ī�޶�, ������=������Ʈ
    if (!io.WantCaptureMouse)
    {
        if (input.IsMouseButtonDown(MouseButton::Left))
        {
            yawCam += input.GetMouseDeltaX() * sens;
            pitchCam += input.GetMouseDeltaY() * sens;
        }
        else if (input.IsMouseButtonDown(MouseButton::Right))
        {
            yawObj += input.GetMouseDeltaX() * sens;
            pitchObj += input.GetMouseDeltaY() * sens;
        }
    }


    // 2) Ű �Է����� ī�޶� �̵� (ī�޶� ȸ�� �ݿ�)
    XMFLOAT3 pos = camera->GetPosition();
    // ���� �� ���
    XMVECTOR camQuat = camera->GetRotationQuat();
    XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), camQuat);
    XMVECTOR right = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), camQuat);
    XMVECTOR up = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), camQuat);

    XMVECTOR posV = XMLoadFloat3(&pos);
    if (input.IsKeyHeld('W')) posV += forward * (moveSpeed * deltaTime);
    if (input.IsKeyHeld('S')) posV -= forward * (moveSpeed * deltaTime);
    if (input.IsKeyHeld('A')) posV -= right * (moveSpeed * deltaTime);
    if (input.IsKeyHeld('D')) posV += right * (moveSpeed * deltaTime);
    if (input.IsKeyHeld('E')) posV -= up * (moveSpeed * deltaTime);
    if (input.IsKeyHeld('Q')) posV += up * (moveSpeed * deltaTime);
    XMStoreFloat3(&pos, posV);
    camera->SetPosition(pos);

    // 3) ī�޶� ȸ�� ���ʹϾ� �������� ����
    XMVECTOR newcamQuat = XMQuaternionRotationRollPitchYaw(pitchCam, yawCam, 0);
    camera->SetRotationQuat(newcamQuat);
    camera->UpdateViewMatrix();

    XMVECTOR objQuat = XMQuaternionRotationRollPitchYaw(pitchObj, yawObj, 0);
    SetRotationQuat(objQuat);

    // 4) ��Ƽ���󡤵���� UI
    if (ImGui::Begin("Sphere Material")) {
        auto& p = materialPBR->parameters;
        ImGui::ColorEdit3("Base Color", reinterpret_cast<float*>(&p.baseColor));
        ImGui::SliderFloat("Metallic", &p.metallic, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular", &p.specular, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &p.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient Occlusion", &p.ambientOcclusion, 0.0f, 1.0f);
        ImGui::ColorEdit3("Emissive Color", reinterpret_cast<float*>(&p.emissiveColor));
        ImGui::SliderFloat("Emissive Intensity", &p.emissiveIntensity, 0.0f, 1.0f);
        
    }

    ImGui::End();
    ImGui::Checkbox("Show Sphere Normal Debug", &showNormalDebug);

    GameObject::Update(deltaTime);
}

void SphereObject::Render(Renderer* renderer)
{

    renderer->SetCamera(camera);

    auto directCommandList = renderer->GetDirectCommandList();

    // descriptor heaps ���ε�
    ID3D12DescriptorHeap* heaps[] = {
        renderer->GetDescriptorHeapManager()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        renderer->GetDescriptorHeapManager()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    directCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // MVP CBV (b0) ������Ʈ
    {
        CB_MVP mvp{};
        XMStoreFloat4x4(&mvp.model, XMMatrixTranspose(worldMatrix));
        XMStoreFloat4x4(&mvp.view, XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix()));
        XMStoreFloat4x4(&mvp.projection, XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
        XMStoreFloat4x4(&mvp.modelInvTranspose, XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));
        memcpy(mappedMVPData, &mvp, sizeof(mvp));
    }

    if (showNormalDebug)
    {
        // --- �븻 ����׿� ���������� ---
        directCommandList->SetGraphicsRootSignature(renderer->GetRootSignatureManager()->Get(L"DebugNormalRS"));

        // b0: MVP CBV
        directCommandList->SetGraphicsRootConstantBufferView(0, constantMVPBuffer->GetGPUVirtualAddress());

        directCommandList->SetPipelineState(renderer->GetPSOManager()->Get(L"DebugNormalPSO"));
        directCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    }
    else
    {
        // --- ���� PBR ������ ---
        directCommandList->SetGraphicsRootSignature(renderer->GetRootSignatureManager()->Get(L"PbrRS"));
        
        // b0: MVP CBV
        directCommandList->SetGraphicsRootConstantBufferView(0, constantMVPBuffer->GetGPUVirtualAddress());
        
        // b1 = ���� CB, b3 = �۷ι� CB,  b4 = ShadowMapViewProj
        directCommandList->SetGraphicsRootConstantBufferView(1, renderer->GetLightingManager()->GetLightingCB()->GetGPUVirtualAddress());
        directCommandList->SetGraphicsRootConstantBufferView(3, renderer->GetGlobalConstantBuffer()->GetGPUVirtualAddress());
        directCommandList->SetGraphicsRootConstantBufferView(4, renderer->GetLightingManager()->GetShadowViewProjCB()->GetGPUVirtualAddress());


        // �˺��� SRV
        if (auto albedoTex = materialPBR->GetAlbedoTexture())
            directCommandList->SetGraphicsRootDescriptorTable(5, albedoTex->GetGpuHandle());
        
        // ȯ��� ���ε� (irradiance, prefiltered, BRDF LUT)
        renderer->GetEnvironmentMaps().Bind(directCommandList, 6, 7, 8);
        
        // ���÷� (s0)
        directCommandList->SetGraphicsRootDescriptorTable(9, renderer->GetDescriptorHeapManager()->GetLinearWrapSamplerGpuHandle());
        
        // shadow map SRV ���̺� ���ε� (t7~t7+N-1) �� root 10
        // shadowMaps[i].srvHandle �� �������� �Ҵ�� NUM_SHADOW_DSV_COUNT ���� SRV�� �����Ƿ�,
        // ù �ڵ鸸 �ѱ�� ��ü ���̺��� ���ε���
        const auto& shadowMaps = renderer->GetShadowMaps();
        directCommandList->SetGraphicsRootDescriptorTable(10, shadowMaps[0].srvHandle.gpuHandle);

        // ��Ƽ���� CBV (b2)
        materialPBR->WriteToConstantBuffer(mappedMaterialBuffer);
        directCommandList->SetGraphicsRootConstantBufferView(2, materialConstantBuffer->GetGPUVirtualAddress());
        
        // �ﰢ�� ����Ʈ�� �׸���
        directCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        directCommandList->SetPipelineState(renderer->GetPSOManager()->Get(L"PbrPSO"));
    }

    // �޽� ���ε� & ��ο� (����)
    directCommandList->IASetVertexBuffers(0, 1, &sphereMesh->GetVertexBufferView());
    directCommandList->IASetIndexBuffer(&sphereMesh->GetIndexBufferView());
    directCommandList->DrawIndexedInstanced(sphereMesh->GetIndexCount(), 1, 0, 0, 0);
}