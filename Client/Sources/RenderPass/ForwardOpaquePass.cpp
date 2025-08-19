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

	// ShadowMap 리소스들 : DEPTH_WRITE → PIXEL_SHADER_RESOURCE
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

	// RTV/DSV 클리어
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


	// ShadowMap 리소스들 : PIXEL_SHADER_RESOURCE → DEPTH_WRITE
	for (const auto& shadowMap : shadowMaps)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap.depthBuffer.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
		commandList->ResourceBarrier(1, &barrier);
	}
}

void ForwardOpaquePass::RecordPreCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer)
{
	FrameResource* frameResource = renderer->GetCurrentFrameResource();

	auto descriptorHeapManager = renderer->GetDescriptorHeapManager();
	UINT backBufferIndex = renderer->GetBackBufferIndex();

	// ShadowMap 리소스들 : DEPTH_WRITE → PIXEL_SHADER_RESOURCE
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

	// RTV/DSV 클리어
	const FLOAT clearColor[4] = { 0, 0, 0, 1 };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

}

void ForwardOpaquePass::RecordParallelCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer, UINT threadIndex)
{
	FrameResource* frameResource = renderer->GetCurrentFrameResource();


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



	const auto& opaqueObjects = renderer->GetOpaqueObjects();

	UINT objectCount = static_cast<UINT>(opaqueObjects.size());
	UINT numThreads = frameResource->numThreads;

	for (UINT i = threadIndex; i < objectCount; i += numThreads)
	{
		opaqueObjects[i]->Render(commandList, renderer, i);
	}
}

void ForwardOpaquePass::RecordPostCommand(ID3D12GraphicsCommandList* commandList, Renderer* renderer)
{
	FrameResource* frameResource = renderer->GetCurrentFrameResource();

	const auto& shadowMaps = frameResource->shadowMaps;

	// ShadowMap 리소스들 : PIXEL_SHADER_RESOURCE → DEPTH_WRITE
	for (const auto& shadowMap : shadowMaps)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap.depthBuffer.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
		commandList->ResourceBarrier(1, &barrier);
	}
}
