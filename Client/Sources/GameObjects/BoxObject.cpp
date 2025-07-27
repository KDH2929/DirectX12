#include "BoxObject.h"
#include "Renderer.h"
#include "DescriptorHeapManager.h"
#include "RootSignatureManager.h"
#include "PipelineStateManager.h"
#include <directx/d3dx12.h>
#include <imgui.h>

using namespace DirectX;

BoxObject::BoxObject(std::shared_ptr<Material> material)
    : materialPBR(std::move(material))
{
}

BoxObject::~BoxObject()
{
    if (materialConstantBuffer && mappedMaterialBuffer) {
        materialConstantBuffer->Unmap(0, nullptr);
    }
}

bool BoxObject::Initialize(Renderer* renderer)
{
    if (!GameObject::Initialize(renderer)) {
        return false;
    }

    // 1) 큐브 메쉬 생성
    cubeMesh = Mesh::CreateCube(renderer);
    if (!cubeMesh) {
        return false;
    }
    SetMesh(cubeMesh);

    // 2) PBR 머티리얼용 상수 버퍼 생성 (256바이트 정렬, Upload heap)
    auto device = renderer->GetDevice();
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(256);
    HRESULT hr = device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&materialConstantBuffer));
    if (FAILED(hr)) {
        return false;
    }

    materialConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedMaterialBuffer));
    materialPBR->WriteToConstantBuffer(mappedMaterialBuffer);

    return true;
}

void BoxObject::Update(float deltaTime)
{
    // ImGui로 PBR 파라미터 조절
    if (ImGui::Begin("Box Material (PBR)"))
    {
        auto& params = materialPBR->parameters;
        ImGui::ColorEdit3("Base Color", reinterpret_cast<float*>(&params.baseColor));
        ImGui::SliderFloat("Metallic", &params.metallic, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &params.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient Occlusion", &params.ambientOcclusion, 0.0f, 1.0f);
        ImGui::ColorEdit3("Emissive Color", reinterpret_cast<float*>(&params.emissiveColor));
        ImGui::SliderFloat("Emissive Intensity", &params.emissiveIntensity, 0.0f, 5.0f);

    }
    ImGui::End();

    GameObject::Update(deltaTime);
}

void BoxObject::Render(Renderer* renderer)
{

    ID3D12GraphicsCommandList* graphicsCommandList = renderer->GetDirectCommandList();
    auto descriptorHeapManager = renderer->GetDescriptorHeapManager();

    ID3D12DescriptorHeap* descriptorHeaps[] = {
        descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    };
    graphicsCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);


    CB_MVP mvpData{};
    XMStoreFloat4x4(&mvpData.model, XMMatrixTranspose(worldMatrix));
    XMStoreFloat4x4(&mvpData.view, XMMatrixTranspose(renderer->GetCamera()->GetViewMatrix()));
    XMStoreFloat4x4(&mvpData.projection, XMMatrixTranspose(renderer->GetCamera()->GetProjectionMatrix()));
    XMStoreFloat4x4(&mvpData.modelInvTranspose, XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix)));
    std::memcpy(mappedMVPData, &mvpData, sizeof(mvpData));


    auto* rootSignature = renderer->GetRootSignatureManager()->Get(L"PbrRS");
    auto* pipelineState = renderer->GetPSOManager()->Get(L"PbrPSO");

    graphicsCommandList->SetGraphicsRootSignature(rootSignature);
    graphicsCommandList->SetPipelineState(pipelineState);

    // b0: MVP
    graphicsCommandList->SetGraphicsRootConstantBufferView(
        0, constantMVPBuffer->GetGPUVirtualAddress());

    // b1: Lighting, b3: Global, b4: ShadowViewProj
    graphicsCommandList->SetGraphicsRootConstantBufferView(
        1, renderer->GetLightingManager()->GetLightingCB()->GetGPUVirtualAddress());
    graphicsCommandList->SetGraphicsRootConstantBufferView(
        3, renderer->GetGlobalConstantBuffer()->GetGPUVirtualAddress());
    graphicsCommandList->SetGraphicsRootConstantBufferView(
        4, renderer->GetLightingManager()->GetShadowViewProjCB()->GetGPUVirtualAddress());

    // PBR 머티리얼 텍스처 바인딩 (t5~)
    if (auto albedoTex = materialPBR->GetAlbedoTexture())
        graphicsCommandList->SetGraphicsRootDescriptorTable(5, albedoTex->GetGpuHandle());

    // 환경맵 (irradiance t6, prefiltered t7, BRDF LUT t8)
    renderer->GetEnvironmentMaps().Bind(
        graphicsCommandList,
        6,
        7,
        8);

    // 샘플러 (s0)
    graphicsCommandList->SetGraphicsRootDescriptorTable(
        9, descriptorHeapManager->GetLinearWrapSamplerGpuHandle());

    // 그림자 맵 SRV 테이블 (t10~)
    auto const& shadowMaps = renderer->GetShadowMaps();
    graphicsCommandList->SetGraphicsRootDescriptorTable(
        10, shadowMaps[0].srvHandle.gpuHandle);

    // b2: MaterialPBR 상수 버퍼
    materialPBR->WriteToConstantBuffer(mappedMaterialBuffer);
    graphicsCommandList->SetGraphicsRootConstantBufferView(
        2, materialConstantBuffer->GetGPUVirtualAddress());

    graphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    graphicsCommandList->IASetVertexBuffers(
        0, 1, &cubeMesh->GetVertexBufferView());
    graphicsCommandList->IASetIndexBuffer(&cubeMesh->GetIndexBufferView());
    graphicsCommandList->DrawIndexedInstanced(
        cubeMesh->GetIndexCount(), 1, 0, 0, 0);
}