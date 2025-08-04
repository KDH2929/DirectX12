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
    // ImGui�� PBR �Ķ���� ����
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

    // PBR ��Ƽ���� ��� ���� ���ε�
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

    // ��Ʈ �ñ״�ó �� ���������� ���� ����
    commandList->SetGraphicsRootSignature(
        renderer->GetRootSignatureManager()->Get(L"PbrRS")
    );
    commandList->SetPipelineState(
        renderer->GetPSOManager()->Get(L"PbrPSO")
    );

    FrameResource* frameResource = renderer->GetCurrentFrameResource();

    // b0: ��-��-���� ��� ����
    commandList->SetGraphicsRootConstantBufferView(
        0, frameResource->cbMVP->GetGPUVirtualAddress(objectIndex)
    );

    // b1: ������ ��� ����
    commandList->SetGraphicsRootConstantBufferView(
        1, frameResource->cbLighting->GetGPUVirtualAddress(0)
    );

    // b2: ��Ƽ���� ��� ����
    commandList->SetGraphicsRootConstantBufferView(
        2, frameResource->cbMaterialPbr->GetGPUVirtualAddress(objectIndex)
    );

    // b3: ���� ��� ����
    commandList->SetGraphicsRootConstantBufferView(
        3, frameResource->cbGlobal->GetGPUVirtualAddress(0)
    );

    // b4: �׸��� �䡤���� ��� ����
    commandList->SetGraphicsRootConstantBufferView(
        4, frameResource->cbShadowViewProj->GetGPUVirtualAddress(0)
    );

    // �ؽ�ó �� ���÷� ���̺� ���ε�
    // �ؽ��İ� ��ȿ�ҽÿ��� ���ε�
    if (materialPBR->GetAlbedoTexture()) {
        commandList->SetGraphicsRootDescriptorTable(
            5, materialPBR->GetAlbedoTexture()->GetGpuHandle()
        );
    }


    renderer->GetEnvironmentMaps().Bind(commandList, 6, 7, 8);
    commandList->SetGraphicsRootDescriptorTable(
        9, descriptorManager->GetLinearWrapSamplerGpuHandle()
    );

    // �׸��ڸ� SRV ���̺� ���ε� (t7~t7+N-1)
    commandList->SetGraphicsRootDescriptorTable(
        10, frameResource->shadowSrv[0].gpuHandle
    );

    // �Է� ����� ���� �� ��ο� ȣ��
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
