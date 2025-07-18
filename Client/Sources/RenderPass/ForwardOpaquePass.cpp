#include "ForwardOpaquePass.h"
#include "Renderer.h"
#include "DescriptorHeapManager.h"


void ForwardOpaquePass::Initialize(Renderer* renderer)
{
}

void ForwardOpaquePass::Update(float deltaTime)
{
}

void ForwardOpaquePass::Render(Renderer* renderer)
{
    auto directCommandList = renderer->GetDirectCommandList();
	auto descriptorHeapManager = renderer->GetDescriptorHeapManager();
	UINT backBufferIndex = renderer->GetBackBufferIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = 
		descriptorHeapManager->GetRtvHandle(static_cast<UINT>(Renderer::RtvIndex::SceneColor));

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
		descriptorHeapManager->GetDsvHandle(static_cast<UINT>(Renderer::DsvIndex::DepthStencil));

	directCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	for (auto& object : renderer->GetOpaqueObjects())
		object->Render(renderer);

}