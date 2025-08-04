#include "ForwardOpaquePass.h"
#include "Renderer.h"
#include "DescriptorHeapManager.h"


void ForwardOpaquePass::Initialize(Renderer* renderer)
{
}

void ForwardOpaquePass::Update(float deltaTime, Renderer* renderer)
{
}

void ForwardOpaquePass::RenderSingleThreaded(Renderer* renderer)
{
	FrameResource* frameResource = renderer->GetCurrentFrameResource();
	ID3D12GraphicsCommandList* commandList = frameResource->commandList.Get();


	auto descriptorHeapManager = renderer->GetDescriptorHeapManager();
	UINT backBufferIndex = renderer->GetBackBufferIndex();

	// ShadowMap ���ҽ��� : DEPTH_WRITE �� PIXEL_SHADER_RESOURCE
	const auto& shadowMaps = frameResource->shadowMaps;
	for (const auto& shadowMap : shadowMaps)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap.depthBuffer.Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &barrier);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = frameResource->sceneColorRtv.cpuHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = frameResource->depthStencilDsv.cpuHandle;

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

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

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissor);

	// RTV/DSV Ŭ����
	const FLOAT clearColor[4] = { 0, 0, 0, 1 };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


	const auto& opaqueObjects = renderer->GetOpaqueObjects();
	for (UINT objectIndex = 0; objectIndex < opaqueObjects.size(); ++objectIndex)
	{
		opaqueObjects[objectIndex]->Render(
			commandList,
			renderer,
			objectIndex);
	}


	// ShadowMap ���ҽ��� : PIXEL_SHADER_RESOURCE �� DEPTH_WRITE
	for (const auto& shadowMap : shadowMaps)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap.depthBuffer.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
		commandList->ResourceBarrier(1, &barrier);
	}
}