// ShaderManager.h
#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "ShaderCompileDesc.h"
#include "ShaderCompiler.h"

using Microsoft::WRL::ComPtr;

class ShaderManager {
public:

    explicit ShaderManager(ID3D12Device* device_);

    bool Init(ID3D12Device* device);
    void Cleanup();

    // ���� ShaderCompileDesc�� �޾Ƽ� �� ���� ������
    bool CompileAll(const std::vector<ShaderCompileDesc>& shaderDescs);

    ComPtr<ID3DBlob> GetShaderBlob(const std::wstring& shaderName) const;
    D3D12_SHADER_BYTECODE GetShaderBytecode(const std::wstring& shaderName) const;


private:
    // �� ���� ShaderCompileDesc�� ShaderCompiler�� ������
    bool Compile(const ShaderCompileDesc& desc, ComPtr<ID3DBlob>& outBlob);

    ID3D12Device* device = nullptr;
    ShaderCompiler compiler;
    std::unordered_map<std::wstring, ComPtr<ID3DBlob>> shaders;
};
