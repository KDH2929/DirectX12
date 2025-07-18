#include "PostProcessPass.h"
#include "Renderer.h"
#include "PostEffects/OutlinePostEffect.h"

void PostProcessPass::Initialize(Renderer* renderer)
{
	auto outline = std::make_shared<OutlinePostEffect>();
	outline->Initialize(renderer);
	postEffects.push_back(outline);
}

void PostProcessPass::Update(float deltaTime)
{
	for (auto& postEffect : postEffects)
	{
		postEffect->Update(deltaTime);
	}
}

void PostProcessPass::Render(Renderer* renderer)
{
	auto directCommandList = renderer->GetDirectCommandList();
	auto descriptorHeapManager = renderer->GetDescriptorHeapManager();
	UINT backBufferIndex = renderer->GetBackBufferIndex();


	auto sceneColorBuffer = renderer->GetSceneColorBuffer();

	// Resource Barrier 로 용도변경
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			sceneColorBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		directCommandList->ResourceBarrier(1, &barrier);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
		descriptorHeapManager->GetRtvHandle(static_cast<UINT>(Renderer::RtvIndex::BackBuffer0) + backBufferIndex);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
		descriptorHeapManager->GetDsvHandle(static_cast<UINT>(Renderer::DsvIndex::DepthStencil));

	directCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);


	for (auto& postEffect : postEffects)
	{
		postEffect->Render(renderer);
	}


	// Resource Barrier 로 용도변경
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			sceneColorBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		directCommandList->ResourceBarrier(1, &barrier);
	}
	
}
