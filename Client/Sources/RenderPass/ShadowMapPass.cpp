#include "ShadowMapPass.h"
#include "Renderer.h"
#include "LightingManager.h"
#include "DescriptorHeapManager.h"
#include "ShadowMap.h"
#include "Lights/BaseLight.h"

void ShadowMapPass::Initialize(Renderer* renderer)
{
}

void ShadowMapPass::Update(float deltaTime, Renderer* renderer)
{
    auto& objects = renderer->GetOpaqueObjects();
    auto& lights = renderer->GetLightingManager()->GetLights();

    UINT objectCount = static_cast<UINT>(objects.size());
    UINT shadowMapIndex = 0;
    
    for (UINT lightIndex = 0; lightIndex < lights.size(); ++lightIndex)
    {
        auto& light = lights[lightIndex];
        if (!light->IsShadowCastingEnabled())
            continue;

        auto viewProjectionMatrices = light->GetShadowViewProjMatrices();
        for (UINT faceIndex = 0; faceIndex < viewProjectionMatrices.size(); ++faceIndex)
        {
            const XMMATRIX& lightViewProjection = viewProjectionMatrices[faceIndex];
            for (UINT objectIndex = 0; objectIndex < objectCount; ++objectIndex)
            {
                objects[objectIndex]->UpdateShadowMap(
                    renderer,
                    objectIndex,
                    shadowMapIndex,
                    lightViewProjection
                );
            }
            ++shadowMapIndex;
        }
    }
}

void ShadowMapPass::RenderSingleThreaded(Renderer* renderer)
{
    auto* frameResource = renderer->GetCurrentFrameResource();

    auto commandList = frameResource->commandList.Get();
    auto& objects = renderer->GetOpaqueObjects();
    auto& lights = renderer->GetLightingManager()->GetLights();
    const UINT objectCount = static_cast<UINT>(objects.size());

    UINT shadowMapIndex = 0;
    const float width = static_cast<float>(SHADOW_MAP_WIDTH);
    const float height = static_cast<float>(SHADOW_MAP_HEIGHT);


    for (UINT lightIndex = 0; lightIndex < lights.size(); ++lightIndex)
    {
        auto& light = lights[lightIndex];
        if (!light->IsShadowCastingEnabled())
            continue;

        auto viewProjMatrices = light->GetShadowViewProjMatrices();
        for (UINT faceIndex = 0; faceIndex < viewProjMatrices.size(); ++faceIndex)
        {
            // viewport & scissor
            D3D12_VIEWPORT     viewport = { 0.0f, 0.0f, width, height, 0.0f, 1.0f };
            D3D12_RECT         scissorRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            // render target & clear
            auto& dsvHandle = frameResource->shadowDsv[shadowMapIndex].cpuHandle;
            commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
            commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

            // draw each object
            for (UINT objectIndex = 0; objectIndex < objectCount; ++objectIndex)
            {
                objects[objectIndex]->RenderShadowMap(
                    commandList,
                    renderer,
                    objectIndex,
                    shadowMapIndex
                );
            }

            ++shadowMapIndex;
        }
    }
}

void ShadowMapPass::RecordParallelCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT threadIndex)
{
    auto* frameResource = renderer->GetCurrentFrameResource();

    auto& objects = renderer->GetOpaqueObjects();
    auto& lights = renderer->GetLightingManager()->GetLights();
    const UINT objectCount = static_cast<UINT>(objects.size());

    UINT shadowMapIndex = 0;
    const float width = static_cast<float>(SHADOW_MAP_WIDTH);
    const float height = static_cast<float>(SHADOW_MAP_HEIGHT);


    for (UINT lightIndex = 0; lightIndex < lights.size(); ++lightIndex)
    {
        auto& light = lights[lightIndex];
        if (!light->IsShadowCastingEnabled())
            continue;

        auto viewProjMatrices = light->GetShadowViewProjMatrices();
        for (UINT faceIndex = 0; faceIndex < viewProjMatrices.size(); ++faceIndex)
        {
            // viewport & scissor
            D3D12_VIEWPORT     viewport = { 0.0f, 0.0f, width, height, 0.0f, 1.0f };
            D3D12_RECT         scissorRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            // render target & clear
            auto& dsvHandle = frameResource->shadowDsv[shadowMapIndex].cpuHandle;
            commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
            commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

            UINT objectCount = static_cast<UINT>(objects.size());
            UINT numThreads = frameResource->numThreads;

            // draw each object
            for (UINT i = threadIndex; i < objectCount; i += numThreads)
            {
                objects[i]->RenderShadowMap(
                    commandList,
                    renderer,
                    i,
                    shadowMapIndex
                );
            }

            ++shadowMapIndex;
        }
    }
}
