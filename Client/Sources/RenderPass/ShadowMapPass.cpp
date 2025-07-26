#include "ShadowMapPass.h"
#include "Renderer.h"
#include "LightingManager.h"
#include "DescriptorHeapManager.h"
#include "ShadowMap.h"
#include "Lights/BaseLight.h"

void ShadowMapPass::Initialize(Renderer* renderer)
{
}

void ShadowMapPass::Update(float deltaTime)
{
}

void ShadowMapPass::Render(Renderer* renderer)
{
    auto commandList = renderer->GetDirectCommandList();
    auto descriptorHeapManager = renderer->GetDescriptorHeapManager();
    auto& shadowMaps = renderer->GetShadowMaps();
    auto lightingManager = renderer->GetLightingManager();



    // ����Ʈ/���� ����
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)SHADOW_MAP_WIDTH, (float)SHADOW_MAP_HEIGHT, 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, (LONG)SHADOW_MAP_WIDTH, (LONG)SHADOW_MAP_HEIGHT };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    // CB_ShadowMapPass ���� ������ۿ� lightViewProj �����ϴ� ���� GameObject���� ����

    
    const auto& lights = lightingManager->GetLights();


    UINT shadowMapIndex = 0;

    for (size_t i = 0; i < lights.size(); ++i)
    {
        auto& light = lights[i];
        if (!light->IsShadowCastingEnabled())
            continue;

        const auto& viewProjMatrices = light->GetShadowViewProjMatrices();
        size_t matrixCount = viewProjMatrices.size();

        for (size_t faceIdx = 0; faceIdx < matrixCount; ++faceIdx)
        {
            const ShadowMap& shadowMap = shadowMaps[shadowMapIndex];

            // DSV ����
            commandList->OMSetRenderTargets(0, nullptr, FALSE, &shadowMap.dsvHandle.cpuHandle);

            // ViewProj ���
            XMMATRIX lightViewProj = viewProjMatrices[faceIdx];

            // ���� Ŭ����
            commandList->ClearDepthStencilView(
                shadowMap.dsvHandle.cpuHandle,
                D3D12_CLEAR_FLAG_DEPTH,
                1.0f, 0,
                0, nullptr
            );

            // ��ο�
            for (auto& object : renderer->GetOpaqueObjects()) {
                object->RenderShadowMapPass(renderer, lightViewProj, shadowMapIndex);
            }

            shadowMapIndex++;
        }
    }
}