#pragma once
#include <wrl/client.h>
#include "ShaderCompileDesc.h"
using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3dcompiler.lib")

class ShaderCompiler {
public:
    bool Compile(const ShaderCompileDesc& desc,
        ComPtr<ID3DBlob>& outBlob,
        ComPtr<ID3DBlob>& outError);
};