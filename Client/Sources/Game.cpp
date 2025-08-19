#include "Game.h"
#include "InputManager.h"
#include "DebugManager.h"
#include "GameObjects/TriangleObject.h"
#include "GameObjects/BoxObject.h"
#include "GameObjects/SphereObject.h"
#include "GameObjects/Flight.h"
#include "GameObjects/Skybox.h"

#include <filesystem>
#include <windowsx.h> 

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);


// 전역 윈도우 프로시저
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return TRUE;
    }

    switch (message) {
    
    case WM_INPUT:
    {
        UINT dataSize;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));
        if (dataSize > 0)
        {
            std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(dataSize);
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer.get(), &dataSize, sizeof(RAWINPUTHEADER)) == dataSize)
            {
                RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.get());
                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    int dx = raw->data.mouse.lLastX;
                    int dy = raw->data.mouse.lLastY;
                    InputManager::GetInstance().OnRawMouseMove(dx, dy);
                }
            }
        }
        break;
    }


    case WM_KEYDOWN:
        if (!(lParam & 0x40000000)) {  // 중복입력 방지
            InputManager::GetInstance().OnKeyDown(static_cast<UINT>(wParam));
        }
        break;

    case WM_KEYUP:
        InputManager::GetInstance().OnKeyUp(static_cast<UINT>(wParam));
        break;

    case WM_MOUSEMOVE:
        InputManager::GetInstance().OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_LBUTTONDOWN:
        InputManager::GetInstance().OnMouseDown(MouseButton::Left);
        break;

    case WM_LBUTTONUP:
        InputManager::GetInstance().OnMouseUp(MouseButton::Left);
        break;

    case WM_RBUTTONDOWN:
        InputManager::GetInstance().OnMouseDown(MouseButton::Right);
        break;

    case WM_RBUTTONUP:
        InputManager::GetInstance().OnMouseUp(MouseButton::Right);
        break;

    case WM_MBUTTONDOWN:
        InputManager::GetInstance().OnMouseDown(MouseButton::Middle);
        break;

    case WM_MBUTTONUP:
        InputManager::GetInstance().OnMouseUp(MouseButton::Middle);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}


bool Game::Initialize(HINSTANCE hInstance, int nCmdShow) {

    // 윈도우 생성
    if (!InitWindow(hInstance, nCmdShow)) {
        MessageBox(nullptr, L"Failed to create window.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!renderer.Initialize(hwnd, windowWidth, windowHeight)) {
        MessageBox(nullptr, L"Failed to initialize Direct3D.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    renderer.InitImGui(hwnd);

    // PhysX 초기화
    if (!physicsManager.Init()) {
        MessageBoxA(nullptr, "PhysX initialization failed", "Error", MB_OK);
        return false;
    }
    PhysicsManager::SetInstance(&physicsManager);

    // 서버 연결 시도
    if (!network.Connect("127.0.0.1", 9999)) {
        MessageBox(nullptr, L"Failed to connect to server.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // 수신 스레드 시작
    network.StartRecvThread();

    InputManager::GetInstance().Initialize(hwnd);
    
    
    // 모델 로드
    LoadModel();
    LoadTexture();
    
    SetupCollisionResponse();

    
    if (!renderer.GetEnvironmentMaps().Load(
        &renderer,
        L"Assets/HDRI/SkyboxDiffuseHDR.dds",   // Irradiance Map
        L"Assets/HDRI/SkyboxSpecularHDR.dds",  // Specular Prefiltered Map
        L"Assets/HDRI/SkyboxBrdf.dds"          // BRDF LUT 2D
    ))
    {
        MessageBox(hwnd, L"Failed to load IBL environment maps!", L"Error", MB_OK);
    }
    



    // 객체 초기화 (테스트 용도)
    
    /*
    auto triangleObject = std::make_shared<TriangleObject>();
    if (!triangleObject->Initialize(&renderer)) {
        throw std::runtime_error("Failed to initialize TriangleObject");
    }
    renderer.AddGameObject(triangleObject);
    */
    
    auto boxMaterial = std::make_shared<Material>();
    boxMaterial->parameters.baseColor = { 1.f, 1.f, 1.f };
    boxMaterial->parameters.ambientOcclusion = 1.0f;
    boxMaterial->parameters.metallic = 0.1f;
    
    auto boxObject = std::make_shared<BoxObject>(boxMaterial);
    if (!boxObject->Initialize(&renderer)) {
        throw std::runtime_error("Failed to initialize BoxObject");
    }
    boxObject->SetPosition(XMFLOAT3{ 0.0f, -2.0f, 0.0f });
    boxObject->SetScale(XMFLOAT3{ 1000.0f,  0.5f, 1000.0f });
    renderer.AddGameObject(boxObject);
    

    // Flight 객체 생성
    /*
    flightMaterial = std::make_shared<Material>();
    flightMaterial->parameters.baseColor = { 1.f, 1.f, 1.f };
    flightMaterial->parameters.ambientOcclusion = 1.0f;
    flightMaterial->SetAllTextures(flightTextures);

    auto flightObject = std::make_shared<Flight>(flight1Mesh, flightTextures);
    if (!flightObject->Initialize(&renderer))
        throw std::runtime_error("Failed to initialize Flight object");

    renderer.AddGameObject(flightObject);
    */

    /*
    auto sphereMaterial = std::make_shared<Material>();
    sphereMaterial->parameters.baseColor = { 1.f, 1.f, 1.f };
    sphereMaterial->parameters.ambientOcclusion = 1.0f;

   
    auto sphereObject = std::make_shared<SphereObject>(sphereMaterial, 32, 32);
    if (!sphereObject->Initialize(&renderer))
        throw std::runtime_error("Failed to initialize SphereObject");

    sphereObject->SetPosition(XMFLOAT3(3.0f, 0.0f, 0.0f));

    renderer.AddGameObject(sphereObject);
    */

    // 성능 테스트를 위한 구 529개 대량배치
    {
        auto sphereMaterial = std::make_shared<Material>();
        sphereMaterial->parameters.baseColor = { 1.f, 1.f, 1.f };
        sphereMaterial->parameters.ambientOcclusion = 1.0f;

        const int   gridCount = 23;      // 23×23 = 529칸
        const float spacing = 2.0f;    // 각 구 좌표 사이 간격
        const float baseY = 0.5f;    // 박스 위에 떠 있도록 Y 위치
        const int maxCount = 529;

        int created = 0;
        for (int z = 0; z < gridCount && created < maxCount; ++z)
        {
            for (int x = 0; x < gridCount && created < maxCount; ++x)
            {
                // SphereObject 인스턴스
                auto sphereObj = std::make_shared<SphereObject>(sphereMaterial, 32, 32);
                if (!sphereObj->Initialize(&renderer))
                    throw std::runtime_error("Failed to initialize SphereObject");

                float offset = (gridCount - 1) * spacing * 0.5f;
                float posX = x * spacing - offset;
                float posZ = z * spacing - offset;

                sphereObj->SetPosition(XMFLOAT3(posX, baseY, posZ));
                renderer.AddGameObject(sphereObj);

                ++created;
            }
        }
    }
    
    auto skybox = std::make_shared<Skybox>(skyboxTexture);
    if (!skybox->Initialize(&renderer)) {
        throw std::runtime_error("Failed to initialize Skybox");
    }

   renderer.AddGameObject(skybox);
    

    return true;
}


void Game::LoadModel()
{
    ModelLoader loader;

    
    std::string flightPath = "Assets/spitfirev6/spitfirev6.obj";
    flight1Mesh = loader.LoadMesh(&renderer, flightPath);
    if (!flight1Mesh) {
        MessageBox(hwnd, L"Failed to load flight mesh!", L"Error", MB_OK);
    }

    std::string bulletPath = "Assets/Bullet/bullet.obj";
    bulletMesh = loader.LoadMesh(&renderer, bulletPath);
    if (!bulletMesh) {
        MessageBox(hwnd, L"Failed to load bullet mesh!", L"Error", MB_OK);
    }
    
}

void Game::LoadTexture()
{
    flightTextures.albedoTexture = renderer.GetTextureManager()->LoadTexture(
        L"Assets/spitfirev6/spitfirev6_Textures/base_Base_Color_1002.png");
    if (!flightTextures.albedoTexture) MessageBox(hwnd, L"Failed to load flight albedo texture!", L"Error", MB_OK);
    
    flightTextures.normalTexture = renderer.GetTextureManager()->LoadTexture(
        L"Assets/spitfirev6/spitfirev6_Textures/base_Normal_DirectX_1002.png");
    if (!flightTextures.normalTexture) MessageBox(hwnd, L"Failed to load flight normal texture!", L"Error", MB_OK);

    flightTextures.metallicTexture = renderer.GetTextureManager()->LoadTexture(
        L"Assets/spitfirev6/spitfirev6_Textures/base_Metallic_1002.png");
    if (!flightTextures.metallicTexture) MessageBox(hwnd, L"Failed to load flight metallic texture!", L"Error", MB_OK);

    flightTextures.roughnessTexture = renderer.GetTextureManager()->LoadTexture(
        L"Assets/spitfirev6/spitfirev6_Textures/base_Roughness_1002.png");
    if (!flightTextures.roughnessTexture) MessageBox(hwnd, L"Failed to load flight roughness texture!", L"Error", MB_OK);

    bulletTexture = renderer.GetTextureManager()->LoadTexture(
        L"Assets/Bullet/Textures/bullet_DefaultMaterial_BaseColor.png");
    if (!bulletTexture) MessageBox(hwnd, L"Failed to load bullet texture!", L"Error", MB_OK);
    

    skyboxTexture = renderer.GetTextureManager()->LoadCubeMap(L"Assets/HDRI/SkyboxSpecularHDR.dds");
    if (!skyboxTexture)
        MessageBox(hwnd, L"Failed to load skybox texture", L"Error", MB_OK);
}

bool Game::InitWindow(HINSTANCE hInstance, int nCmdShow) {
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC, WndProc,
        0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr,
        windowClassName, nullptr
    };

    if (!RegisterClassEx(&wc)) {
        MessageBox(nullptr, L"Failed to register window class", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    RECT wr = { 0, 0, windowWidth, windowHeight };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd = CreateWindow(windowClassName, windowTitle,
        WS_OVERLAPPEDWINDOW, windowPosX, windowPosY,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) {
        MessageBox(nullptr, L"Failed to create window", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 타이머 초기화
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&prevTime);

    return true;
}


int Game::Run() {
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {

            LARGE_INTEGER curTime;
            QueryPerformanceCounter(&curTime);
            deltaTime = static_cast<float>(curTime.QuadPart - prevTime.QuadPart) / frequency.QuadPart;
            prevTime = curTime;

            renderer.ImGuiNewFrame();

            HandleInput(deltaTime);  // 키보드와 마우스 입력 처리
            ProcessNetwork();  // 서버에서 받은 데이터 반영
            Update(deltaTime);      // 게임 로직 갱신

            ImGui::Render();

            physicsManager.Simulate(deltaTime);
            physicsManager.FetchResults();
            physicsManager.ApplyActorRemovals();


            Render();      // D3D 렌더링
        }
    }

    Cleanup();
    return static_cast<int>(msg.wParam);
}


void Game::Cleanup() {
    physicsManager.Cleanup();
    network.Stop();
}

void Game::Update(float deltaTime) {

    totalTime += deltaTime;

    renderer.UpdateGlobalTime(totalTime);
    renderer.Update(deltaTime);

    DebugManager::GetInstance().Update(deltaTime);
    InputManager::GetInstance().Update(deltaTime);
}

void Game::Render() {

    renderer.Render();          // D3D 렌더링
}

void Game::HandleInput(float deltaTime) {
    auto& input = InputManager::GetInstance();

    if (input.IsKeyJustPressed(VK_SPACE)) {
        network.Send("FIRE");
    }
}

void Game::ProcessNetwork() {
    // 서버 패킷 처리
}

void Game::SetupCollisionResponse()
{
    // Player ↔ Enemy 충돌
    physicsManager.SetCollisionResponse(CollisionLayer::Player, CollisionLayer::Enemy, CollisionResponse::Block);

    // Player ↔ EnemyProjectile 충돌
    physicsManager.SetCollisionResponse(CollisionLayer::Player, CollisionLayer::EnemyProjectile, CollisionResponse::Block);
    physicsManager.SetCollisionResponse(CollisionLayer::EnemyProjectile, CollisionLayer::Player, CollisionResponse::Block);

    // Enemy ↔ PlayerProjectile 충돌
    physicsManager.SetCollisionResponse(CollisionLayer::Enemy, CollisionLayer::PlayerProjectile, CollisionResponse::Block);
    physicsManager.SetCollisionResponse(CollisionLayer::PlayerProjectile, CollisionLayer::Enemy, CollisionResponse::Block);

    // Ignore 자기 팀 탄환과 본체
    physicsManager.SetCollisionResponse(CollisionLayer::PlayerProjectile, CollisionLayer::Player, CollisionResponse::Ignore);
    physicsManager.SetCollisionResponse(CollisionLayer::EnemyProjectile, CollisionLayer::Enemy, CollisionResponse::Ignore);

    // Ignore 탄환끼리 충돌
    physicsManager.SetCollisionResponse(CollisionLayer::PlayerProjectile, CollisionLayer::EnemyProjectile, CollisionResponse::Ignore);
    physicsManager.SetCollisionResponse(CollisionLayer::PlayerProjectile, CollisionLayer::PlayerProjectile, CollisionResponse::Ignore);
    physicsManager.SetCollisionResponse(CollisionLayer::EnemyProjectile, CollisionLayer::EnemyProjectile, CollisionResponse::Ignore);

    // Default → Projectile 충돌도 원하면 설정
    physicsManager.SetCollisionResponse(CollisionLayer::Default, CollisionLayer::PlayerProjectile, CollisionResponse::Block);
    physicsManager.SetCollisionResponse(CollisionLayer::Default, CollisionLayer::EnemyProjectile, CollisionResponse::Block);
}
