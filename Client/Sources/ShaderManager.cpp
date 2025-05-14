#include "ShaderManager.h"
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

bool ShaderManager::Init(ID3D11Device* device_) {
    device = device_;
    return device != nullptr;
}

void ShaderManager::Cleanup() {
    shaders.clear();     // ComPtr 로 리소스는 알아서 해제됨
    device = nullptr;
}

bool ShaderManager::CompileShaders()
{
    std::wstring vertexShaderPath = L"Shaders\\TriangleVertexShader.hlsl";
    std::wstring pixelShaderPath = L"Shaders\\TrianglePixelShader.hlsl";

    // TriangleVertexShader 의 inputLayout
    D3D11_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    // 셰이더 컴파일
    if (!CompileVertexShader(vertexShaderPath, L"TriangleVertexShader", inputLayout, ARRAYSIZE(inputLayout), "VSMain")) {
        return false;
    }

    if (!CompilePixelShader(pixelShaderPath, L"TrianglePixelShader", "PSMain")) {
        return false;
    }


    // Phong PS-VS 컴파일
    std::wstring phongVSPath = L"Shaders\\PhongVertexShader.hlsl";
    std::wstring phongPSPath = L"Shaders\\PhongPixelShader.hlsl";

    D3D11_INPUT_ELEMENT_DESC phongInputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,  D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    if (!CompileVertexShader(phongVSPath, L"PhongVertexShader", phongInputLayout, ARRAYSIZE(phongInputLayout), "VSMain"))
        return false;

    if (!CompilePixelShader(phongPSPath, L"PhongPixelShader", "PSMain"))
        return false;


    // Skybox 셰이더 등록
    std::wstring skyboxVSPath = L"Shaders\\SkyboxVertexShader.hlsl";
    std::wstring skyboxPSPath = L"Shaders\\SkyboxPixelShader.hlsl";

    OutputDebugStringW((L"Trying to load shader from: " + skyboxVSPath + L"\n").c_str());

    // Skybox InputLayout: POSITION만 필요
    D3D11_INPUT_ELEMENT_DESC skyboxLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if (!CompileVertexShader(skyboxVSPath, L"SkyboxVertexShader", skyboxLayout, ARRAYSIZE(skyboxLayout), "VSMain"))
        return false;

    if (!CompilePixelShader(skyboxPSPath, L"SkyboxPixelShader", "PSMain"))
        return false;


    std::wstring flashVSPath = L"Shaders\\FlashVertexShader.hlsl";
    std::wstring flashPSPath = L"Shaders\\FlashPixelShader.hlsl";

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if (!CompileVertexShader(flashVSPath, L"FlashVertexShader", layout, ARRAYSIZE(layout), "VSMain"))
        return false;

    if (!CompilePixelShader(flashPSPath, L"FlashPixelShader", "PSMain"))
        return false;


    std::wstring smokeVSPath = L"Shaders\\SmokeVertexShader.hlsl";
    std::wstring smokePSPath = L"Shaders\\SmokePixelShader.hlsl";

    D3D11_INPUT_ELEMENT_DESC smokeLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if (!CompileVertexShader(smokeVSPath, L"SmokeVertexShader", smokeLayout, ARRAYSIZE(smokeLayout), "VSMain"))
        return false;

    if (!CompilePixelShader(smokePSPath, L"SmokePixelShader", "PSMain"))
        return false;



    std::wstring wireframeVSPath = L"Shaders\\WireframeShader.hlsl";
    std::wstring wireframePSPath = L"Shaders\\WireframeShader.hlsl";

    D3D11_INPUT_ELEMENT_DESC wfLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if (!CompileVertexShader(wireframeVSPath, L"WireframeShader", wfLayout, ARRAYSIZE(wfLayout), "VSMain"))
        return false;

    if (!CompilePixelShader(wireframePSPath, L"WireframeShader", "PSMain"))
        return false;

    
    std::wstring ui2DVSPath = L"Shaders\\UI2DVertexShader.hlsl";
    std::wstring ui2DPSPath = L"Shaders\\UI2DPixelShader.hlsl";

    D3D11_INPUT_ELEMENT_DESC ui2DLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if (!CompileVertexShader(ui2DVSPath, L"UI2DVertexShader", ui2DLayout, ARRAYSIZE(ui2DLayout), "VSMain"))
        return false;

    if (!CompilePixelShader(ui2DPSPath, L"UI2DPixelShader", "PSMain"))
        return false;


    std::wstring explosionPSPath = L"Shaders\\ExplosionPixelShader.hlsl";

    if (!CompilePixelShader(explosionPSPath, L"ExplosionPixelShader", "PSMain"))
        return false;

    return true;
}

bool ShaderManager::CompileVertexShader(
    const std::wstring& vsPath,
    const std::wstring& shaderName,
    const D3D11_INPUT_ELEMENT_DESC* inputLayoutDescArray,
    UINT numElements,
    const std::string& entry,
    const std::string& profile
) {
    if (!device) return false;

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompileFromFile(
        vsPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry.c_str(), profile.c_str(),
        0, 0, &vsBlob, &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    ShaderData& shaderData = shaders[shaderName];

    hr = device->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &shaderData.vertexShader
    );
    if (FAILED(hr)) {
        return false;
    }

    hr = device->CreateInputLayout(
        inputLayoutDescArray,
        numElements,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &shaderData.inputLayout
    );

    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool ShaderManager::CompilePixelShader(
    const std::wstring& psPath,
    const std::wstring& shaderName,
    const std::string& entry,
    const std::string& profile
) {
    if (!device) return false;

    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompileFromFile(
        psPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry.c_str(), profile.c_str(),
        0, 0, &psBlob, &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    ShaderData& shaderData = shaders[shaderName];

    hr = device->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &shaderData.pixelShader
    );

    if (FAILED(hr)) {
        return false;
    }

    return true;
}

ID3D11VertexShader* ShaderManager::GetVertexShader(const std::wstring& name) const {
    auto it = shaders.find(name);
    return (it != shaders.end() && it->second.vertexShader) ? it->second.vertexShader.Get() : nullptr;
}

ID3D11PixelShader* ShaderManager::GetPixelShader(const std::wstring& name) const {
    auto it = shaders.find(name);
    return (it != shaders.end() && it->second.pixelShader) ? it->second.pixelShader.Get() : nullptr;
}

ID3D11InputLayout* ShaderManager::GetInputLayout(const std::wstring& name) const {
    auto it = shaders.find(name);
    return (it != shaders.end() && it->second.inputLayout) ? it->second.inputLayout.Get() : nullptr;
}
