#pragma once

#include <string>
#include <vector>
#include <d3d12.h>
#include <d3dcompiler.h>

struct ShaderCompileDesc {
    std::wstring           name;        // 캐시 맵의 키
    std::wstring           path;        // HLSL 파일 경로
    std::string            entryPoint;  // e.g. "VSMain"
    std::string            profile;     // e.g. "vs_5_0"
    UINT                   compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
    std::vector<D3D_SHADER_MACRO> defines;
};