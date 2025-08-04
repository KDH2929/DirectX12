#include "Flight.h"
#include "Renderer.h"
#include "Mesh.h"
#include "TextureManager.h"
#include "Material.h"
#include "FrameResource/FrameResource.h"
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

    // 텍스처 리소스 상태 전환
    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    assert(frameResource && "CurrentFrameResource is null");

    ID3D12CommandAllocator* commandAllocator = frameResource->commandAllocator.Get();
    ID3D12GraphicsCommandList* commandList = frameResource->commandList.Get();
    ID3D12CommandQueue* commandQueue = renderer->GetDirectQueue();

    ThrowIfFailed(commandAllocator->Reset());
    ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));

    for (auto& texturePtr : {
             materialPBR->GetAlbedoTexture(),
             materialPBR->GetNormalTexture(),
             materialPBR->GetMetallicTexture(),
             materialPBR->GetRoughnessTexture() })
    {
        if (!texturePtr)
            continue;

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            texturePtr->GetResource(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        commandList->ResourceBarrier(1, &barrier);
    }

    ThrowIfFailed(commandList->Close());
    ID3D12CommandList* lists[] = { commandList };
    commandQueue->ExecuteCommandLists(_countof(lists), lists);
    renderer->WaitForDirectQueue();

    return true;
}

void Flight::Update(float deltaTime, Renderer* renderer, UINT objectIndex)
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

    // 3) ImGui: Flight material parameters
    if (ImGui::Begin("Flight Material"))
    {
        auto& parameters = materialPBR->parameters;
        ImGui::ColorEdit3("Base Color", reinterpret_cast<float*>(&parameters.baseColor));
        ImGui::SliderFloat("Metallic", &parameters.metallic, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular", &parameters.specular, 0.0f, 10.0f);
        ImGui::SliderFloat("Roughness", &parameters.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient Occlusion", &parameters.ambientOcclusion, 0.0f, 1.0f);
        ImGui::ColorEdit3("Emissive Color", reinterpret_cast<float*>(&parameters.emissiveColor));
        ImGui::SliderFloat("Emissive Intensity", &parameters.emissiveIntensity, 0.0f, 10.0f);
    }
    ImGui::End();

    // 4) Material constant-buffer upload
    CB_MaterialPBR materialConstantData{};
    auto& parameters = materialPBR->parameters;
    materialConstantData.baseColor = parameters.baseColor;
    materialConstantData.metallic = parameters.metallic;
    materialConstantData.specular = parameters.specular;
    materialConstantData.roughness = parameters.roughness;
    materialConstantData.ambientOcclusion = parameters.ambientOcclusion;
    materialConstantData.emissiveColor = parameters.emissiveColor;
    materialConstantData.emissiveIntensity = parameters.emissiveIntensity;

    uint32_t textureFlags = 0;
    if (materialPBR->GetAlbedoTexture())    textureFlags |= USE_ALBEDO_MAP;
    if (materialPBR->GetNormalTexture())    textureFlags |= USE_NORMAL_MAP;
    if (materialPBR->GetMetallicTexture())  textureFlags |= USE_METALLIC_MAP;
    if (materialPBR->GetRoughnessTexture()) textureFlags |= USE_ROUGHNESS_MAP;
    materialConstantData.flags = textureFlags;


    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    assert(frameResource && frameResource->cbMaterialPbr && "FrameResource or cbMaterialPbr is null");
    frameResource->cbMaterialPbr->CopyData(objectIndex, materialConstantData);


    GameObject::Update(deltaTime, renderer, objectIndex);
}


void Flight::Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex)
{
    // Descriptor Heaps 바인딩 (CBV_SRV_UAV + SAMPLER)
    ID3D12DescriptorHeap* descriptorHeaps[] = {
        renderer->GetDescriptorHeapManager()->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        renderer->GetDescriptorHeapManager()->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // PBR  RS & PSO 바인딩
    ID3D12RootSignature* pbrRootSignature = renderer->GetRootSignatureManager()->Get(L"PbrRS");
    ID3D12PipelineState* pbrPso = renderer->GetPSOManager()->Get(L"PbrPSO");
    commandList->SetGraphicsRootSignature(pbrRootSignature);
    commandList->SetPipelineState(pbrPso);

    // CBV 바인딩 (b0: MVP, b1: Lighting, b2: Material, b3: Global, b4: ShadowMapViewProj)
    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    commandList->SetGraphicsRootConstantBufferView(
        0, frameResource->cbMVP->GetGPUVirtualAddress(objectIndex));
    commandList->SetGraphicsRootConstantBufferView(
        1, frameResource->cbLighting->GetGPUVirtualAddress(0));
    commandList->SetGraphicsRootConstantBufferView(
        2, frameResource->cbMaterialPbr->GetGPUVirtualAddress(objectIndex));
    commandList->SetGraphicsRootConstantBufferView(
        3, frameResource->cbGlobal->GetGPUVirtualAddress(0));
    commandList->SetGraphicsRootConstantBufferView(
        4, frameResource->cbShadowViewProj->GetGPUVirtualAddress(0));

    // SRV 및 Sampler 테이블 바인딩 (root slot 5~10)
    {
        // material textures (t0~t3) → root 5
        commandList->SetGraphicsRootDescriptorTable(
            5, materialPBR->GetAlbedoTexture()->GetGpuHandle());

        // IBL 환경맵 바인딩 (t4~t6)
        renderer->GetEnvironmentMaps().Bind(
            commandList, 6, 7, 8);

        // sampler (s0, s1) → root 9
        commandList->SetGraphicsRootDescriptorTable(
            9, renderer->GetDescriptorHeapManager()->GetLinearWrapSamplerGpuHandle());

        // shadow map SRV 테이블 바인딩 (t7~t7+N-1) → root 10
        commandList->SetGraphicsRootDescriptorTable(
            10, frameResource->shadowSrv[0].gpuHandle);
    }

    // IA 설정 및 Draw Call
    if (auto meshInstance = flightMesh.lock())
    {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &meshInstance->GetVertexBufferView());
        commandList->IASetIndexBuffer(&meshInstance->GetIndexBufferView());
        commandList->DrawIndexedInstanced(
            meshInstance->GetIndexCount(), 1, 0, 0, 0);
    }
}