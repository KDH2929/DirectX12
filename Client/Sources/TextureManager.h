#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "Texture.h"

class Renderer;
class DescriptorHeapManager;

class TextureManager
{
public:

    void Initialize(Renderer* renderer_,
        DescriptorHeapManager* descriptorHeapManager_);

    // 텍스처 로드 및 캐시
    std::shared_ptr<Texture> LoadTexture(const std::wstring& filePath);
    void Clear();

private:
    Renderer* renderer = nullptr;
    DescriptorHeapManager* descriptorHeapManager = nullptr;
    std::unordered_map<std::wstring, std::shared_ptr<Texture>> textureCache;
};