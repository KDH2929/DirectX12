#include "BoxObject.h"
#include "Renderer.h"
#include "DescriptorHeapManager.h"
#include "RootSignatureManager.h"
#include "PipelineStateManager.h"
#include "EnvironmentMaps.h"
#include "FrameResource/FrameResource.h"
#include <directx/d3dx12.h>
#include <imgui.h>
#include <cassert>

using namespace DirectX;

BoxObject::BoxObject(std::shared_ptr<Material> material)
    : materialPBR(std::move(material))
{
}

bool BoxObject::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer))
    {
        return false;
    }

    cubeMesh = Mesh::CreateCube(renderer);
    if (!cubeMesh)
    {
        return false;
    }
    SetMesh(cubeMesh);

    return true;
}

void BoxObject::Update(float deltaTime, Renderer* renderer, UINT objectIndex)
{
    // ImGui로 PBR 파라미터 조절
    if (ImGui::Begin("Box Material (PBR)"))
    {
        auto& parameters = materialPBR->parameters;
        ImGui::ColorEdit3("Base Color", reinterpret_cast<float*>(&parameters.baseColor));
        ImGui::SliderFloat("Metallic", &parameters.metallic, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &parameters.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient Occlusion", &parameters.ambientOcclusion, 0.0f, 1.0f);
        ImGui::ColorEdit3("Emissive Color", reinterpret_cast<float*>(&parameters.emissiveColor));
        ImGui::SliderFloat("Emissive Intensity", &parameters.emissiveIntensity, 0.0f, 5.0f);
    }
    ImGui::End();

    // PBR 머티리얼 상수 버퍼 업로드
    CB_MaterialPBR materialData{};
    auto& params = materialPBR->parameters;
    materialData.baseColor = params.baseColor;
    materialData.metallic = params.metallic;
    materialData.specular = params.specular;
    materialData.roughness = params.roughness;
    materialData.ambientOcclusion = params.ambientOcclusion;
    materialData.emissiveColor = params.emissiveColor;
    materialData.emissiveIntensity = params.emissiveIntensity;

    FrameResource* frameResource = renderer->GetCurrentFrameResource();
    assert(frameResource && frameResource->cbMaterialPbr && "FrameResource or cbMaterialPbr is null");
    frameResource->cbMaterialPbr->CopyData(objectIndex, materialData);

    GameObject::Update(deltaTime, renderer, objectIndex);
}

void BoxObject::Render(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT objectIndex)
{
    auto descriptorManager = renderer->GetDescriptorHeapManager();
    ID3D12DescriptorHeap* descriptorHeaps[] = {
        descriptorManager->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        descriptorManager->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // 루트 시그니처 및 파이프라인 상태 설정
    commandList->SetGraphicsRootSignature(
        renderer->GetRootSignatureManager()->Get(L"PbrRS")
    );
    commandList->SetPipelineState(
        renderer->GetPSOManager()->Get(L"PbrPSO")
    );

    FrameResource* frameResource = renderer->GetCurrentFrameResource();

    // b0: 모델-뷰-투영 상수 버퍼
    commandList->SetGraphicsRootConstantBufferView(
        0, frameResource->cbMVP->GetGPUVirtualAddress(objectIndex)
    );

    // b1: 라이팅 상수 버퍼
    commandList->SetGraphicsRootConstantBufferView(
        1, frameResource->cbLighting->GetGPUVirtualAddress(0)
    );

    // b2: 머티리얼 상수 버퍼
    commandList->SetGraphicsRootConstantBufferView(
        2, frameResource->cbMaterialPbr->GetGPUVirtualAddress(objectIndex)
    );

    // b3: 전역 상수 버퍼
    commandList->SetGraphicsRootConstantBufferView(
        3, frameResource->cbGlobal->GetGPUVirtualAddress(0)
    );

    // b4: 그림자 뷰·투영 상수 버퍼
    commandList->SetGraphicsRootConstantBufferView(
        4, frameResource->cbShadowViewProj->GetGPUVirtualAddress(0)
    );

    // 텍스처 및 샘플러 테이블 바인딩
    // 텍스쳐가 유효할시에만 바인딩
    if (materialPBR->GetAlbedoTexture()) {
        commandList->SetGraphicsRootDescriptorTable(
            5, materialPBR->GetAlbedoTexture()->GetGpuHandle()
        );
    }


    renderer->GetEnvironmentMaps().Bind(commandList, 6, 7, 8);
    commandList->SetGraphicsRootDescriptorTable(
        9, descriptorManager->GetLinearWrapSamplerGpuHandle()
    );

    // 그림자맵 SRV 테이블 바인딩 (t7~t7+N-1)
    commandList->SetGraphicsRootDescriptorTable(
        10, frameResource->shadowSrv[0].gpuHandle
    );

    // 입력 어셈블러 설정 및 드로우 호출
    if (cubeMesh)
    {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &cubeMesh->GetVertexBufferView());
        commandList->IASetIndexBuffer(&cubeMesh->GetIndexBufferView());
        commandList->DrawIndexedInstanced(
            cubeMesh->GetIndexCount(), 1, 0, 0, 0
        );
    }
}
