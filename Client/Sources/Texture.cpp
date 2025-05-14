#include "Texture.h"

Texture::Texture(ComPtr<ID3D11ShaderResourceView> srv, const std::wstring& name)
    : shaderResourceView(srv), name(name)
{
}

ID3D11ShaderResourceView* Texture::GetShaderResourceView() const
{
    return shaderResourceView.Get();
}

const std::wstring& Texture::GetName() const
{
    return name;
}
