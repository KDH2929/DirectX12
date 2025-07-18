#pragma once

#include "NetworkManager.h"
#include "PhysicsManager.h"
#include "Renderer.h"
#include "ModelLoader.h"
#include "Material.h"

#include <windows.h>



class Game {
public:
    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    int Run();          // �޽��� ���� + ������ ����
    void Cleanup();

private:
    HWND hwnd;
    
    // ������ ���� ����
    int windowWidth = 1280;
    int windowHeight = 720;
    int windowPosX = 100;
    int windowPosY = 100;
    LPCWSTR windowClassName = L"D3DWindowClass";
    LPCWSTR windowTitle = L"Direct3D Game Window";

    LARGE_INTEGER prevTime;     // ���� �������� �ð�
    LARGE_INTEGER frequency;    // ���� ī������ ���ļ�
    float deltaTime = 0.0f;         // ������ �� �ð� ����
    float totalTime = 0.0f;

    NetworkManager network;
    PhysicsManager physicsManager;
    Renderer renderer;
    ModelLoader modelLoader;

    // Meshes
    std::shared_ptr<Mesh> flight1Mesh;
    std::shared_ptr<Material> flightMaterial;
    MaterialPbrTextures flightTextures;

    // Skybox
    std::shared_ptr<Texture> skyboxTexture;

    std::vector<XMFLOAT3> enemyPositions = {
        { -60.0f, 50.0f, 100.0f },
        { -30.0f, 50.0f, 120.0f },
        {   0.0f, 50.0f, 130.0f },
        {  30.0f, 50.0f, 120.0f },
        {  60.0f, 50.0f, 100.0f },
    };


    std::shared_ptr<Mesh> bulletMesh;
    std::shared_ptr<Texture> bulletTexture;


    void LoadModel();
    void LoadTexture();

    bool InitWindow(HINSTANCE hInstance, int nCmdShow);
    void Update(float deltaTime);
    void Render();
    void HandleInput(float deltaTime);      // ������ Ű����� ���콺 �Է°� ����
    void ProcessNetwork();                   // ������ ���� ��Ŷ ó��

    void SetupCollisionResponse();
};
