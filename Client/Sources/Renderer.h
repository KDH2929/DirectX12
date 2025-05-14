#pragma once

#include "GameObject.h"
#include "ShaderManager.h"
#include "Camera.h"
#include "Light.h"
#include "ConstantBuffers.h"

#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include <memory>

#pragma comment(lib, "d3d11.lib")

using namespace Microsoft::WRL;

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Init(HWND hwnd, int width, int height);
    void Cleanup();

    ID3D11Device* GetDevice() const;
    ID3D11DeviceContext* GetDeviceContext() const;
    ShaderManager* GetShaderManager();

    void AddGameObject(std::shared_ptr<GameObject> obj);
    void RemoveGameObject(std::shared_ptr<GameObject> obj);

    void Update(float deltaTime);
    void Render();

    bool InitGlobalBuffer();
    void UpdateGlobalTime(float totalTime);

    Camera* GetCamera();
    void SetCamera(std::shared_ptr<Camera> newCamera);

    ID3D11Buffer* GetLightingConstantBuffer();
    ID3D11SamplerState* GetSamplerState() const;

    bool SetupLightingConstantBuffer();
    bool SetupTextureSampler();
    void SetupDefaultLights();
    


    int GetViewportWidth() const;
    int GetViewportHeight() const;

private:
    bool InitD3D(HWND hwnd, int width, int height);


private:
    // Direct3D 관련 변수들
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> deviceContext;
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11RenderTargetView> renderTargetView;
    ComPtr<ID3D11DepthStencilView> depthStencilView;
    ComPtr<ID3D11DepthStencilState> depthStencilState;
    D3D11_VIEWPORT viewport;


    std::vector<std::shared_ptr<GameObject>> gameObjects;
    std::shared_ptr<ShaderManager> shaderManager;

    std::shared_ptr<Camera> mainCamera;

    CB_Global globalCB;
    ComPtr<ID3D11Buffer> cbGlobalBuffer;

    std::vector<Light> lights;
    ComPtr<ID3D11Buffer> lightingConstantBuffer;
    CB_Lighting lightingData;


    ComPtr<ID3D11SamplerState> samplerState;

};
