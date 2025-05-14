#pragma once

#include <wrl/client.h>
#include <d3d11.h>
#include <string>

using Microsoft::WRL::ComPtr;

class Texture {
public:
    Texture(ComPtr<ID3D11ShaderResourceView> srv, const std::wstring& name = L"");

    ID3D11ShaderResourceView* GetShaderResourceView() const;
    const std::wstring& GetName() const;

private:
    ComPtr<ID3D11ShaderResourceView> shaderResourceView;
    std::wstring name;
};
