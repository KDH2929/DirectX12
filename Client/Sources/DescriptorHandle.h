#pragma once
#include <d3d12.h>
#include <wrl/client.h>

struct DescriptorHandle
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ 0 };
    UINT index = 0;   // offset inside the heap
};
