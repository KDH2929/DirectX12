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

	// ShadowMap 리소스들 : DEPTH_WRITE → PIXEL_SHADER_RESOURCE
	const auto& shadowMaps = renderer->GetShadowMaps();
	for (const auto& shadowMap : shadowMaps)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap.depthBuffer.Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		directCommandList->ResourceBarrier(1, &barrier);
	}
	

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = 
		descriptorHeapManager->GetRtvCpuHandle(static_cast<UINT>(Renderer::RtvIndex::SceneColor));

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
		descriptorHeapManager->GetDsvCpuHandle(static_cast<UINT>(Renderer::DsvIndex::DepthStencil));

	directCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	D3D12_VIEWPORT viewport = {
		0.0f, 0.0f,
		static_cast<float>(renderer->GetViewportWidth()),
		static_cast<float>(renderer->GetViewportHeight()),
		0.0f, 1.0f
	};

	D3D12_RECT scissor = {
		0, 0,
		static_cast<LONG>(renderer->GetViewportWidth()),
		static_cast<LONG>(renderer->GetViewportHeight())
	};

	directCommandList->RSSetViewports(1, &viewport);
	directCommandList->RSSetScissorRects(1, &scissor);

	// RTV/DSV 클리어
	const FLOAT clearColor[4] = { 0, 0, 0, 1 };
	directCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	directCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


	for (auto& object : renderer->GetOpaqueObjects()) {
		object->Render(renderer);
	}


	// ShadowMap 리소스들 : PIXEL_SHADER_RESOURCE → DEPTH_WRITE
	for (const auto& shadowMap : shadowMaps)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap.depthBuffer.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
		directCommandList->ResourceBarrier(1, &barrier);
	}
	
}