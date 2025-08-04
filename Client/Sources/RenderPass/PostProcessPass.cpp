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

void PostProcessPass::Update(float deltaTime, Renderer* renderer)
{
	for (auto& postEffect : postEffects)
	{
		postEffect->Update(deltaTime, renderer);
	}
}

void PostProcessPass::RenderSingleThreaded(Renderer* renderer)
{
	FrameResource* frameResource = renderer->GetCurrentFrameResource();
	ID3D12GraphicsCommandList* commandList = frameResource->commandList.Get();

	auto descriptorHeapManager = renderer->GetDescriptorHeapManager();
	UINT backBufferIndex = renderer->GetBackBufferIndex();

	auto& swapChainRtvs = renderer->GetSwapChainRtvs();

	// Resource Barrier 로 용도변경
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			frameResource->sceneColorBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandList->ResourceBarrier(1, &barrier);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapChainRtvs[backBufferIndex].cpuHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = frameResource->depthStencilDsv.cpuHandle;

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);


	for (auto& postEffect : postEffects)
	{
		postEffect->Render(renderer);
	}


	// Resource Barrier 로 용도변경
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			frameResource->sceneColorBuffer.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandList->ResourceBarrier(1, &barrier);
	}
	
}
