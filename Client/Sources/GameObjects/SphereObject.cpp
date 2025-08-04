#include "SphereObject.h"
#include "Renderer.h"
#include "Mesh.h"
#include "FrameResource/FrameResource.h"
#include <directx/d3dx12.h>
#include <imgui.h>
#include <cassert>

using namespace DirectX;

SphereObject::SphereObject(
    std::shared_ptr<Material> material,
    uint32_t latitudeSeg,
    uint32_t longitudeSeg
)
    : latitudeSegments(latitudeSeg)
    , longitudeSegments(longitudeSeg)
    , materialPBR(std::move(material))
{
}

bool SphereObject::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
        return false;

    // 카메라 생성 및 설정
    camera = std::make_shared<Camera>();
    camera->SetPosition({ 0.0f, 0.0f, -10.0f });
    float aspect = float(renderer->GetViewportWidth()) / float(renderer->GetViewportHeight());
    camera->SetPerspective(XM_PIDIV4, aspect, 0.1f, 1000.0f);

    // 구 메시 생성
    sphereMesh = Mesh::CreateSphere(renderer, latitudeSegments, longitudeSegments);
    if (!sphereMesh)
        return false;
    SetMesh(sphereMesh);

    return true;
}

void SphereObject::Update(
    float     deltaTime,
    Renderer* renderer,
    UINT      objectIndex
)
{
    auto& input = InputManager::GetInstance();
    ImGuiIO& io = ImGui::GetIO();
    const float sensitivity = 0.002f;

    if (!io.WantCaptureMouse)
    {
        if (input.IsMouseButtonDown(MouseButton::Left))
        {
            yawCam += input.GetMouseDeltaX() * sensitivity;
            pitchCam -= input.GetMouseDeltaY() * sensitivity;
        }
        else if (input.IsMouseButtonDown(MouseButton::Right))
        {
            yawObj += input.GetMouseDeltaX() * sensitivity;
            pitchObj -= input.GetMouseDeltaY() * sensitivity;
        }
    }

    XMFLOAT3 cameraPos = camera->GetPosition();
    XMVECTOR posVec = XMLoadFloat3(&cameraPos);
    XMVECTOR camQuat = camera->GetRotationQuat();
    XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), camQuat);
    XMVECTOR right = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), camQuat);
    XMVECTOR up = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), camQuat);

    if (input.IsKeyHeld('W')) posVec += forward * (moveSpeed * deltaTime);
    if (input.IsKeyHeld('S')) posVec -= forward * (moveSpeed * deltaTime);
    if (input.IsKeyHeld('A')) posVec -= right * (moveSpeed * deltaTime);
    if (input.IsKeyHeld('D')) posVec += right * (moveSpeed * deltaTime);
    if (input.IsKeyHeld('E')) posVec -= up * (moveSpeed * deltaTime);
    if (input.IsKeyHeld('Q')) posVec += up * (moveSpeed * deltaTime);

    XMFLOAT3 newPos; XMStoreFloat3(&newPos, posVec);
    camera->SetPosition(newPos);

    camera->SetRotationQuat(XMQuaternionRotationRollPitchYaw(-pitchCam, yawCam, 0));
    camera->UpdateViewMatrix();
    SetRotationQuat(XMQuaternionRotationRollPitchYaw(-pitchObj, yawObj, 0));

    if (ImGui::Begin("Sphere Material"))
    {
        auto& parameters = materialPBR->parameters;
        ImGui::ColorEdit3("Base Color", reinterpret_cast<float*>(&parameters.baseColor));
        ImGui::SliderFloat("Metallic", &parameters.metallic, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular", &parameters.specular, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &parameters.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient Occlusion", &parameters.ambientOcclusion, 0.0f, 1.0f);
        ImGui::ColorEdit3("Emissive Color", reinterpret_cast<float*>(&parameters.emissiveColor));
        ImGui::SliderFloat("Emissive Intensity", &parameters.emissiveIntensity, 0.0f, 5.0f);
    }
    ImGui::End();
    ImGui::Checkbox("Show Sphere Normal Debug", &showNormalDebug);


    CB_MaterialPBR materialData{};
    auto& parameters = materialPBR->parameters;
    materialData.baseColor = parameters.baseColor;
    materialData.metallic = parameters.metallic;
    materialData.specular = parameters.specular;
    materialData.roughness = parameters.roughness;
    materialData.ambientOcclusion = parameters.ambientOcclusion;
    materialData.emissiveColor = parameters.emissiveColor;
    materialData.emissiveIntensity = parameters.emissiveIntensity;

    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    assert(frameResource && frameResource->cbMaterialPbr && "FrameResource or cbMaterialPbr is null");
    frameResource->cbMaterialPbr->CopyData(objectIndex, materialData);

    renderer->SetCamera(camera);

    GameObject::Update(deltaTime, renderer, objectIndex);

}

void SphereObject::Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex)
{
    // Descriptor Heaps 바인딩
    ID3D12DescriptorHeap* heaps[] = {
        renderer->GetDescriptorHeapManager()->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        renderer->GetDescriptorHeapManager()->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    FrameResource* frameResource = renderer->GetCurrentFrameResource();

    // 디버그 노말 파이프라인
    if (showNormalDebug)
    {

        commandList->SetGraphicsRootSignature(
            renderer->GetRootSignatureManager()->Get(L"DebugNormalRS")
        );
        commandList->SetPipelineState(
            renderer->GetPSOManager()->Get(L"DebugNormalPSO")
        );

        commandList->SetGraphicsRootConstantBufferView(0, frameResource->cbMVP->GetGPUVirtualAddress(objectIndex));
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    }

    // PBR 렌더링
    else
    {
        commandList->SetGraphicsRootSignature(
            renderer->GetRootSignatureManager()->Get(L"PbrRS")
        );
        commandList->SetPipelineState(
            renderer->GetPSOManager()->Get(L"PbrPSO")
        );

        
        commandList->SetGraphicsRootConstantBufferView(0, frameResource->cbMVP->GetGPUVirtualAddress(objectIndex)); commandList->SetGraphicsRootConstantBufferView(1, frameResource->cbLighting->GetGPUVirtualAddress(0)); commandList->SetGraphicsRootConstantBufferView(3, frameResource->cbGlobal->GetGPUVirtualAddress(0)); commandList->SetGraphicsRootConstantBufferView(4, frameResource->cbShadowViewProj->GetGPUVirtualAddress(0));

        if (materialPBR->GetAlbedoTexture()) {
            commandList->SetGraphicsRootDescriptorTable(
                5, materialPBR->GetAlbedoTexture()->GetGpuHandle()
            );
        }

        renderer->GetEnvironmentMaps().Bind(commandList, 6, 7, 8);
        commandList->SetGraphicsRootDescriptorTable(9, renderer->GetDescriptorHeapManager()->GetLinearWrapSamplerGpuHandle());
        commandList->SetGraphicsRootDescriptorTable(10, frameResource->shadowSrv[0].gpuHandle);

        commandList->SetGraphicsRootConstantBufferView(2, frameResource->cbMaterialPbr->GetGPUVirtualAddress(objectIndex));
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    // IA 설정 및 드로우
    commandList->IASetVertexBuffers(0, 1, &sphereMesh->GetVertexBufferView());
    commandList->IASetIndexBuffer(&sphereMesh->GetIndexBufferView());
    commandList->DrawIndexedInstanced(sphereMesh->GetIndexCount(), 1, 0, 0, 0);
}