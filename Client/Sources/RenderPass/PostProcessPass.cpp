#include "PostProcessPass.h"
#include "Renderer.h"
#include "PostEffects/OutlinePostEffect.h"
#include "PostEffects/ToneMappingPostEffect.h"

void PostProcessPass::Initialize(Renderer* renderer)
{
	// ����� PostProcess �ϳ��� ó���ϵ��� �����Ƿ� ���� ���� �����ʿ�
	auto outline = std::make_shared<OutlinePostEffect>();
	outline->Initialize(renderer);
	// postEffects.push_back(outline);

	// Tone Mapping ȿ��
	// Tone Mapping �� ����� �츮�� ���ؼ��� HDR �̹���, �� Float16 ����Ÿ�ٿ� �������� �ؾ���
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

	// Resource Barrier �� �뵵����
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


	// Resource Barrier �� �뵵����
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			sceneColorBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		directCommandList->ResourceBarrier(1, &barrier);
	}
	
}
