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
}

bool Flight::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
        return false;

    camera = std::make_shared<Camera>();
    camera->SetPosition({ 0.0f, 0.0f, -10.0f });
    float aspect = float(renderer->GetViewportWidth())
        / float(renderer->GetViewportHeight());
    camera->SetPerspective(XM_PIDIV4, aspect, 0.1f, 1000.0f);


    ID3D12Device* device = renderer->GetDevice();
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC   bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(256);
    if (FAILED(device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&materialConstantBuffer))))
        return false;
    materialConstantBuffer->Map(
        0, nullptr,
        reinterpret_cast<void**>(&mappedMaterialBuffer));
    materialPBR->WriteToGpu(mappedMaterialBuffer);

    // 텍스처 리소스 상태 전환
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

    // 1) 마우스 드래그: 왼쪽=카메라, 오른쪽=오브젝트
    if (!io.WantCaptureMouse)
    {
        if (input.IsMouseButtonDown(MouseButton::Left))
        {
            /*
            // 카메라 yaw/pitch
            yawCam += input.GetMouseDeltaX() * sens;
            pitchCam += -input.GetMouseDeltaY() * sens;
            pitchCam = std::clamp(pitchCam, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);

            // 카메라 쿼터니언 갱신 & 뷰 매트릭스
            XMVECTOR camQuat = XMQuaternionRotationRollPitchYaw(
                -pitchCam, yawCam, 0.0f);
            camera->SetRotationQuat(camQuat);
            camera->UpdateViewMatrix();
            */
        }
    }

    // 2) 키 입력으로 카메라 이동 (카메라 회전 반영)
    {
        // 로컬 축 벡터 계산
        XMVECTOR camQuat = camera->GetRotationQuat();
        XMVECTOR forward = XMVector3Rotate(
            XMVectorSet(0, 0, 1, 0), camQuat);
        XMVECTOR right = XMVector3Rotate(
            XMVectorSet(1, 0, 0, 0), camQuat);
        XMVECTOR upWorld = XMVectorSet(0, 1, 0, 0); // 월드 업 고정

        // 위치 가져와서 이동
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

    // 3) ImGui: Flight 머티리얼 파라미터
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
    // 카메라 셋팅
    // renderer->SetCamera(camera);

    // 1) Direct command list 가져오기
    ID3D12GraphicsCommandList* directCommandList =
        renderer->GetDirectCommandList();

    // 2) Descriptor Heaps 바인딩 (CBV_SRV_UAV + SAMPLER)
    ID3D12DescriptorHeap* descriptorHeaps[] = {
        renderer->GetDescriptorHeapManager()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        renderer->GetDescriptorHeapManager()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    directCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);


    // DebugNormalRS & PSO 바인딩
    directCommandList->SetGraphicsRootSignature(
        renderer->GetRootSignatureManager()->Get(L"DebugNormalRS"));
    directCommandList->SetGraphicsRootConstantBufferView(
        0, constantMVPBuffer->GetGPUVirtualAddress());
    directCommandList->SetPipelineState(
        renderer->GetPSOManager()->Get(L"DebugNormalPSO"));
    // 포인트 리스트로 법선만 찍기
    directCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

    // 3) PBR 전용 RS & PSO 바인딩
    directCommandList->SetGraphicsRootSignature(
        renderer->GetRootSignatureManager()->Get(L"PbrRS"));
    directCommandList->SetPipelineState(
        renderer->GetPSOManager()->Get(L"PbrPSO"));

    // 4) 프레임 전역 CBV 바인딩 (b1: Lighting, b3: Global)
    directCommandList->SetGraphicsRootConstantBufferView(
        1, renderer->GetLightingConstantBuffer()->GetGPUVirtualAddress());
    directCommandList->SetGraphicsRootConstantBufferView(
        3, renderer->GetGlobalConstantBuffer()->GetGPUVirtualAddress());

    // 5) SRV 및 Sampler 테이블 바인딩 (root slot 4~8)
    {
        // material textures (t0~t3) → root 4
        directCommandList->SetGraphicsRootDescriptorTable(
            4, materialPBR->GetAlbedoTexture()->GetGpuHandle());

        // IBL 환경맵 바인딩 (t4~t6)
        renderer->GetEnvironmentMaps().Bind(
            directCommandList,
            5,  // Irradiance
            6,  // Specular
            7   // BrdfLut
        );

        // sampler (s0, s1)
        directCommandList->SetGraphicsRootDescriptorTable(
            8, renderer->GetDescriptorHeapManager()->GetWrapSamplerHandle());
        // directCommandList->SetGraphicsRootDescriptorTable(9, renderer->GetDescriptorHeapManager()->GetClampSamplerHandle());
    }

    // 6) MVP CBV (b0) 업데이트 & 바인딩
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

    // 7) Material CBV (b2) 업데이트 & 바인딩
    {
        materialPBR->WriteToGpu(mappedMaterialBuffer);
        directCommandList->SetGraphicsRootConstantBufferView(
            2, materialConstantBuffer->GetGPUVirtualAddress());
    }

    // 8) IA 설정 및 Draw 호출
    if (auto mesh = flightMesh.lock())
    {
        directCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        directCommandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        directCommandList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        directCommandList->DrawIndexedInstanced(
            mesh->GetIndexCount(), 1, 0, 0, 0);
    }

}