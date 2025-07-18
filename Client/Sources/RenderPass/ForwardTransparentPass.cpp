#include "ForwardTransparentPass.h"
#include "Renderer.h"
#include "DescriptorHeapManager.h"

void ForwardTransparentPass::Initialize(Renderer* renderer)
{
}

void ForwardTransparentPass::Update(float deltaTime)
{
}

void ForwardTransparentPass::Render(Renderer* renderer)
{
	auto directCommandList = renderer->GetDirectCommandList();
	auto descriptorHeapManager = renderer->GetDescriptorHeapManager();
	UINT backBufferIndex = renderer->GetBackBufferIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
		descriptorHeapManager->GetRtvHandle(static_cast<UINT>(Renderer::RtvIndex::SceneColor));

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
		descriptorHeapManager->GetDsvHandle(static_cast<UINT>(Renderer::DsvIndex::DepthStencil));

	directCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	for (auto& object : renderer->GetTransparentObjects())
		object->Render(renderer);
}
