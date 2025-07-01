#include "ShaderManager.h"
#include <stdexcept>

ShaderManager::ShaderManager(ID3D12Device* device_)
    : device(device_)
{
    if (!device)
        throw std::invalid_argument("ShaderManager requires a valid ID3D12Device");
}

bool ShaderManager::Init(ID3D12Device* device_) {
    if (!device_) return false;
    device = device_;
    return true;
}

void ShaderManager::Cleanup() {
    shaders.clear();
    device = nullptr;
}

// ShaderCompiler를 활용한 단일 컴파일
bool ShaderManager::Compile(
    const ShaderCompileDesc& desc,
    ComPtr<ID3DBlob>& outBlob)
{
    ComPtr<ID3DBlob> errorBlob;
    if (!compiler.Compile(desc, outBlob, errorBlob)) {
        if (errorBlob)
            OutputDebugStringA(
                static_cast<const char*>(errorBlob->GetBufferPointer()));
    
#if defined(_DEBUG)
        throw std::runtime_error("Shader compilation failed: " +
            std::string(desc.path.begin(), desc.path.end()));
#endif
        return false;
    }
    return true;
}

// 여러 쉐이더를 한 번에 컴파일
bool ShaderManager::CompileAll(
    const std::vector<ShaderCompileDesc>& shaderDescs)
{
    shaders.clear();
    for (const auto& desc : shaderDescs) {
        ComPtr<ID3DBlob> blob;
        if (!Compile(desc, blob))
            return false;
        shaders[desc.name] = blob;
    }
    return true;
}


ComPtr<ID3DBlob> ShaderManager::GetShaderBlob(const std::wstring& shaderName) const
{
    auto it = shaders.find(shaderName);
    if (it == shaders.end()) {
        throw std::runtime_error(
            "Shader not found: " +
            std::string(shaderName.begin(), shaderName.end()));
    }
    return it->second; // ComPtr<ID3DBlob> 복사(레퍼런스 카운트 증가)
}


D3D12_SHADER_BYTECODE ShaderManager::GetShaderBytecode(const std::wstring& shaderName) const
{
    auto blob = GetShaderBlob(shaderName);
    return { blob->GetBufferPointer(), blob->GetBufferSize() };
}