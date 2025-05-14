#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <wrl/client.h>
#include <d3d11.h>
#include "Texture.h" 

using Microsoft::WRL::ComPtr;

class TextureManager {
public:
    void Initialize(ID3D11Device* device);

    std::shared_ptr<Texture> LoadTexture(const std::wstring& filePath);

    void Clear();

private:
    ID3D11Device* device = nullptr;
    std::unordered_map<std::wstring, std::shared_ptr<Texture>> textureCache;
};