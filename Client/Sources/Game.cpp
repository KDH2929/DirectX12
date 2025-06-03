#include "Game.h"
#include "InputManager.h"
#include "DebugManager.h"
#include "TriangleObject.h"
#include "BoxObject.h"
#include "Flight.h"
#include "BillboardMuzzleFlash.h"
#include "BillboardMuzzleSmoke.h"
#include "BillboardExplosion.h"

#include <filesystem>
#include <windowsx.h> 

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);


// 전역 윈도우 프로시저
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
        return true;        // ImGui가 메시지를 처리했다면, 더 이상 처리하지 않음
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


bool Game::Init(HINSTANCE hInstance, int nCmdShow) {

    // 윈도우 생성
    if (!InitWindow(hInstance, nCmdShow)) {
        MessageBox(nullptr, L"Failed to create window.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!renderer.Init(hwnd, windowWidth, windowHeight)) {
        MessageBox(nullptr, L"Failed to initialize Direct3D.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!InitImGui()) {
        MessageBox(nullptr, L"Failed to initialize ImGui.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

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
    textureManager.Initialize(renderer.GetDevice());

    // 모델 로드
    LoadModel();
    LoadTexture();
    
    SetupCollisionResponse();


    // 객체 초기화 (테스트 용도)
    // std::shared_ptr<TriangleObject> triangleObject = std::make_shared<TriangleObject>();
    // triangleObject->Initialize(&renderer);
    // renderer.AddGameObject(triangleObject);


    // Skybox
    skybox = std::make_shared<Skybox>(L"Assets/Cubemaps/skybox.dds"); // 큐브맵 텍스처 경로
    if (!skybox->Initialize(&renderer)) {
        MessageBox(nullptr, L"Failed to initialize Skybox.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    renderer.AddGameObject(skybox);


    // HeightMapTerrain 생성
    terrain = std::make_shared<HeightMapTerrain>(
        L"Assets/Heightmaps/iceland_heightmap.png",
        0, 0,                                 // width, height
        50.0f,                                 // heightScale
        1.0f                                  // vertexDistance
    );

    // terrain->SetPosition(XMFLOAT3(-512.0f, -40.0f, -512.0f));
    terrain->SetPosition(XMFLOAT3(-40.0f, -40.0f, -40.0f));
    terrain->Initialize(&renderer);
    renderer.AddGameObject(terrain);


    // 테스트 용도
    // std::shared_ptr<BoxObject> boxObject = std::make_shared<BoxObject>();
    // boxObject->Initialize(&renderer);
    // renderer.AddGameObject(boxObject);

    
    /*
    auto muzzleFlashTexture = textureManager.LoadTexture(L"Assets/Textures/MuzzleFlash2.png");
    std::shared_ptr<BillboardMuzzleFlash> muzzleFlash1 = std::make_shared<BillboardMuzzleFlash>();
    muzzleFlash1->Initialize(&renderer, muzzleFlashTexture, 10.0f, 20.0f);
    muzzleFlash1->SetFadeOut(false);
    renderer.AddGameObject(muzzleFlash1);
    */

    auto smokeTexture = textureManager.LoadTexture(L"Assets/Textures/MuzzleSmoke3.png");
    auto smokeNoiseTexture = textureManager.LoadTexture(L"Assets/NoiseTextures/noise2.png");

    auto smoke = std::make_shared<BillboardMuzzleSmoke>();
    smoke->Initialize(&renderer, smokeTexture, smokeNoiseTexture, 20.0f, 50.5f);
    smoke->SetFadeOut(false);
    renderer.AddGameObject(smoke);

    
    playerFlight1 = std::make_shared<PlayerFlight>(flight1Mesh, flight1Texture);
    playerFlight1->Initialize(&renderer);
    renderer.AddGameObject(playerFlight1);


    // 폭발 텍스처 로드
    auto explosionTexture = textureManager.LoadTexture(L"Assets/Textures/ExplosionSpriteSheet.png");  // 폭발 텍스처

    // 폭발 이펙트 스프라이트 생성
    auto explosion = std::make_shared<BillboardExplosion>();
    explosion->Initialize(&renderer, explosionTexture, 4, 5, 2.0f);  // 스프라이트 시트 (4x5)로 설정, 폭발 애니메이션 지속 시간
    explosion->SetSize(10.0f);
    explosion->DeActivate();

    renderer.AddGameObject(explosion);

    for (const auto& pos : enemyPositions) {
        auto enemy = std::make_shared<EnemyFlight>(flight1Mesh, flight1Texture);
        enemy->SetPosition(pos);
        enemy->Initialize(&renderer);
        enemy->SetExplosionEffect(explosion);
        renderer.AddGameObject(enemy);
        enemyFlights.push_back(enemy);
    }


    

    // 테스트용도
    /*
    XMFLOAT3 spawnPos = { 0.0f, 30.0f, 0.0f };
    XMFLOAT3 direction = { 0.0f, 0.0f, 1.0f };
    float speed = 0.0f;

    auto bullet = std::make_shared<Bullet>(spawnPos, direction, speed, bulletMesh, bulletTexture);
    bullet->Initialize(&renderer);
    bullet->SetMaxLifeTime(999.0f);
    renderer.AddGameObject(bullet);
    */

    renderer.SetupDefaultLights();


    auto crosshairTexture = textureManager.LoadTexture(L"Assets/UI/crosshair.png");

    float screenWidth = static_cast<float>(renderer.GetViewportWidth());
    float screenHeight = static_cast<float>(renderer.GetViewportHeight());

    XMFLOAT2 crosshairPosition(screenWidth / 2.0f, screenHeight / 2.0f);  // 화면 중앙
    XMFLOAT2 crosshairSize(25.0f, 25.0f);

    // UI2D 객체 생성
    crosshair = std::make_shared<UI2D>(crosshairPosition, crosshairSize, crosshairTexture);

    // UI2D 객체 초기화
    if (!crosshair->Initialize(&renderer)) {
        return false;  // 초기화 실패
    }

    renderer.AddGameObject(crosshair);

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
    flight1Texture = textureManager.LoadTexture(L"Assets/spitfirev6/spitfirev6_Textures/base_Base_Color_1002.png");
    bulletTexture = textureManager.LoadTexture(L"Assets/Bullet/Textures/bullet_DefaultMaterial_BaseColor.png");
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

bool Game::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(float(windowWidth), float(windowHeight));
    ImGui::StyleColorsDark();

    if (!ImGui_ImplDX11_Init(renderer.GetDevice(), renderer.GetDeviceContext())) {
        return false;
    }

    if (!ImGui_ImplWin32_Init(hwnd)) {
        return false;
    }
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

            HandleInput(deltaTime);  // 키보드와 마우스 입력 처리
            ProcessNetwork();  // 서버에서 받은 데이터 반영
            Update(deltaTime);      // 게임 로직 갱신

            physicsManager.Simulate(deltaTime);
            physicsManager.FetchResults();
            physicsManager.ApplyActorRemovals();

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            Render();      // D3D 렌더링
        }
    }

    Cleanup();
    return static_cast<int>(msg.wParam);
}


void Game::Cleanup() {

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

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
