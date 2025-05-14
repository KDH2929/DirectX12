#pragma once

#include "ConstantBuffers.h"
#include "ShaderInfo.h"
#include "Mesh.h"
#include "Texture.h"
#include "TextureManager.h"
#include "PhysicsManager.h"
#include "ModelLoader.h"

#include <d3d11.h>
#include <memory>
#include <DirectXMath.h>

using namespace DirectX;

class Renderer;

class GameObject {
public:
    GameObject();
    virtual ~GameObject() = default;

    virtual bool Initialize(Renderer* renderer);
    virtual void Update(float deltaTime) = 0;
    virtual void Render(Renderer* renderer) = 0;

    void UpdateWorldMatrix();
    void SetPosition(const XMFLOAT3& position);
    void SetScale(const XMFLOAT3& scale);

    void SetRotationEuler(const XMFLOAT3& eulerAngles);    // Euler -> Quaternion 변환
    void SetRotationQuat(const XMVECTOR& quat);            // Quaternion 설정

    XMFLOAT3 GetPosition() const;
    XMFLOAT3 GetScale() const;

    XMFLOAT3 GetRotationEuler() const;
    XMVECTOR GetRotationQuat() const;

    void AddForce(const XMFLOAT3& force);

    void SetColliderOffset(const XMFLOAT3& offset);
    XMFLOAT3 GetColliderOffset() const;

    physx::PxRigidActor* GetPhysicsActor() const;
    void SetCollisionLayer(CollisionLayer layer);

protected:

    void BindPhysicsActor(physx::PxRigidActor* actor);
    void SyncFromPhysics();
    void ApplyTransformToPhysics();
    void DrawPhysicsColliderDebug();


protected:
    XMFLOAT3 position;
    XMFLOAT3 scale;

    XMFLOAT3 rotationEuler;
    XMVECTOR rotationQuat;          // 실제 회전 값

    XMMATRIX worldMatrix = XMMatrixIdentity();

    ComPtr<ID3D11Buffer> constantMVPBuffer;
    ComPtr<ID3D11Buffer> constantMaterialBuffer;

    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11RasterizerState> rasterizerState;
    ComPtr<ID3D11BlendState> blendState;
    ComPtr<ID3D11DepthStencilState> depthStencilState;

    CB_Material materialData;
    std::shared_ptr<Texture> diffuseTexture;

    ShaderInfo shaderInfo;
    TextureManager textureManager;
    ModelLoader modelLoader;

    physx::PxRigidActor* physicsActor = nullptr;
    XMFLOAT3 colliderOffset = { 0, 0, 0 };

};
