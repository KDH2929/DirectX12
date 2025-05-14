#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <string>
#include <unordered_map>

using namespace Microsoft::WRL;

class ShaderManager {
public:
    bool Init(ID3D11Device* device);
    void Cleanup();

    bool CompileShaders();

    bool CompileVertexShader(
        const std::wstring& vsPath,
        const std::wstring& shaderName,
        const D3D11_INPUT_ELEMENT_DESC* inputLayoutDescArray,
        UINT numElements,
        const std::string& entry = "VSMain",
        const std::string& profile = "vs_5_0"
    );

    bool CompilePixelShader(
        const std::wstring& psPath,
        const std::wstring& shaderName,
        const std::string& entry = "PSMain",
        const std::string& profile = "ps_5_0"
    );

    ID3D11VertexShader* GetVertexShader(const std::wstring& name) const;
    ID3D11PixelShader* GetPixelShader(const std::wstring& name) const;
    ID3D11InputLayout* GetInputLayout(const std::wstring& name) const;

private:
    struct ShaderData {
        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
        ComPtr<ID3D11InputLayout> inputLayout;
    };

    std::unordered_map<std::wstring, ShaderData> shaders;
    ID3D11Device* device = nullptr;
};
