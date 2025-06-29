// ShaderCompiler.cpp
#include "ShaderCompiler.h"
#include <d3dcompiler.h>

bool ShaderCompiler::Compile(
    const ShaderCompileDesc& desc,
    ComPtr<ID3DBlob>& outBlob,
    ComPtr<ID3DBlob>& outError)
{
    // ��ũ�� �迭 + �� ����
    std::vector<D3D_SHADER_MACRO> macros = desc.defines;
    macros.push_back({ nullptr, nullptr });

    // ���� ������ ȣ��
    HRESULT hr = D3DCompileFromFile(
        desc.path.c_str(),
        macros.data(),
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        desc.entryPoint.c_str(),
        desc.profile.c_str(),
        desc.compileFlags,
        0,
        outBlob.ReleaseAndGetAddressOf(),
        outError.ReleaseAndGetAddressOf()
    );

    return SUCCEEDED(hr);
}
