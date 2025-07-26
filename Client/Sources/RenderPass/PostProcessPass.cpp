#include "PostProcessPass.h"
#include "Renderer.h"
#include "PostEffects/OutlinePostEffect.h"
#include "PostEffects/ToneMappingPostEffect.h"

void PostProcessPass::Initialize(Renderer* renderer)
{
	// 현재는 PostProcess 하나만 처리하도록 했으므로 추후 구조 수정필요
	auto outline = std::make_shared<OutlinePostEffect>();
	outline->Initialize(renderer);
	// postEffects.push_back(outline);

	// Tone Mapping 효과
	// Tone Mapping 을 제대로 살리기 위해서는 HDR 이미지, 즉 Float16 렌더타겟에 렌더링을 해야함
	auto toneMapping = std::make_shared<ToneMappingPostEffect>();
	toneMapping->Initialize(renderer);
	postEffects.push_back(toneMapping);
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
		descriptorHeapManager->GetRtvCpuHandle(static_cast<UINT>(Renderer::RtvIndex::BackBuffer0) + backBufferIndex);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
		descriptorHeapManager->GetDsvCpuHandle(static_cast<UINT>(Renderer::DsvIndex::DepthStencil));

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
