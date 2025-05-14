#pragma once

#include "GameObject.h"
#include <string>

class Skybox : public GameObject {
public:
    Skybox(const std::wstring& cubeMapPath);
    virtual bool Initialize(Renderer* renderer) override;
    virtual void Update(float deltaTime) override;
    virtual void Render(Renderer* renderer) override;

private:
    void SetDepthState(ID3D11Device* device);
    void SetRasterizerState(ID3D11Device* device);

    std::wstring cubeMapPath;
    std::shared_ptr<Mesh> cubeMesh;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeTextureSRV;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
};