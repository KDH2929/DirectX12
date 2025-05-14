#pragma once

#include <string>
#include <d3d11.h>  

struct ShaderInfo {
    std::wstring vertexShaderName;
    std::wstring pixelShaderName;
    D3D11_CULL_MODE cullMode = D3D11_CULL_BACK;
    bool useAlphaBlending = false;
    bool useDepthTest = true;
};
