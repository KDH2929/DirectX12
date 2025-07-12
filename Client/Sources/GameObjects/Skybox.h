#pragma once

#include "GameObject.h"
#include "Mesh.h"
#include "Texture.h"
#include "ConstantBuffers.h"  
#include <wrl/client.h>
#include <memory>
#include <string>

using Microsoft::WRL::ComPtr;

class Renderer;

class Skybox : public GameObject
{
public:
    Skybox(std::shared_ptr<Texture> cubeMapTexture);
    ~Skybox() override = default;

    bool Initialize(Renderer* renderer) override;
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

private:
    std::shared_ptr<Mesh>     cubeMesh;
    std::shared_ptr<Texture>  cubeMapTexture;

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;
};

