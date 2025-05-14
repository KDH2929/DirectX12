#include "TextureManager.h"
#include <DirectXTK/WICTextureLoader.h>
#include <wrl/client.h>

using namespace DirectX;

void TextureManager::Initialize(ID3D11Device* device) {
    this->device = device;
}

std::shared_ptr<Texture> TextureManager::LoadTexture(const std::wstring& filePath) {
    auto it = textureCache.find(filePath);
    if (it != textureCache.end()) {
        return it->second;
    }

    ComPtr<ID3D11ShaderResourceView> srv;
    HRESULT hr = CreateWICTextureFromFile(
        device,
        filePath.c_str(),
        nullptr,
        srv.GetAddressOf()
    );

    if (FAILED(hr)) {
        OutputDebugStringW((L"Failed to load texture: " + filePath + L"\n").c_str());
        return nullptr;
    }

    auto texture = std::make_shared<Texture>(srv, filePath);
    textureCache[filePath] = texture;
    return texture;
}

void TextureManager::Clear() {
    textureCache.clear();
}
    