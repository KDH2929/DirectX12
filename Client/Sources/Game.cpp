#include "Game.h"
#include "InputManager.h"
#include "DebugManager.h"
#include "TriangleObject.h"
#include "BoxObject.h"

#include <filesystem>
#include <windowsx.h> 

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);


// ���� ������ ���ν���
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
        return true;        // ImGui�� �޽����� ó���ߴٸ�, �� �̻� ó������ ����
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
        if (!(lParam & 0x40000000)) {  // �ߺ��Է� ����
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

    // ������ ����
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

    // PhysX �ʱ�ȭ
    if (!physicsManager.Init()) {
        MessageBoxA(nullptr, "PhysX initialization failed", "Error", MB_OK);
        return false;
    }
    PhysicsManager::SetInstance(&physicsManager);

    // ���� ���� �õ�
    if (!network.Connect("127.0.0.1", 9999)) {
        MessageBox(nullptr, L"Failed to connect to server.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // ���� ������ ����
    network.StartRecvThread();

    InputManager::GetInstance().Initialize(hwnd);
    //textureManager.Initialize(renderer.GetDevice());

    // �� �ε�
    //LoadModel();
    //LoadTexture();
    
    SetupCollisionResponse();


    // ��ü �ʱ�ȭ (�׽�Ʈ �뵵)
    /*
    auto triangleObject = std::make_shared<TriangleObject>();
    if (!triangleObject->Initialize(&renderer)) {
        throw std::runtime_error("Failed to initialize TriangleObject");
    }
    renderer.AddGameObject(triangleObject);
    */

    auto boxObject = std::make_shared<BoxObject>();
    if (!boxObject->Initialize(&renderer)) {
        throw std::runtime_error("Failed to initialize BoxObject");
    }
    renderer.AddGameObject(boxObject);

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

    // Ÿ�̸� �ʱ�ȭ
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

    /*
    if (!ImGui_ImplDX11_Init(renderer.GetDevice(), renderer.GetDeviceContext())) {
        return false;
    }
    */

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

            HandleInput(deltaTime);  // Ű����� ���콺 �Է� ó��
            ProcessNetwork();  // �������� ���� ������ �ݿ�
            Update(deltaTime);      // ���� ���� ����

            physicsManager.Simulate(deltaTime);
            physicsManager.FetchResults();
            physicsManager.ApplyActorRemovals();

            /*
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            */

            Render();      // D3D ������
        }
    }

    Cleanup();
    return static_cast<int>(msg.wParam);
}


void Game::Cleanup() {

    // ImGui_ImplDX11_Shutdown();
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

    renderer.Render();          // D3D ������
}

void Game::HandleInput(float deltaTime) {
    auto& input = InputManager::GetInstance();

    if (input.IsKeyJustPressed(VK_SPACE)) {
        network.Send("FIRE");
    }
}

void Game::ProcessNetwork() {
    // ���� ��Ŷ ó��
}

void Game::SetupCollisionResponse()
{
    // Player �� Enemy �浹
    physicsManager.SetCollisionResponse(CollisionLayer::Player, CollisionLayer::Enemy, CollisionResponse::Block);

    // Player �� EnemyProjectile �浹
    physicsManager.SetCollisionResponse(CollisionLayer::Player, CollisionLayer::EnemyProjectile, CollisionResponse::Block);
    physicsManager.SetCollisionResponse(CollisionLayer::EnemyProjectile, CollisionLayer::Player, CollisionResponse::Block);

    // Enemy �� PlayerProjectile �浹
    physicsManager.SetCollisionResponse(CollisionLayer::Enemy, CollisionLayer::PlayerProjectile, CollisionResponse::Block);
    physicsManager.SetCollisionResponse(CollisionLayer::PlayerProjectile, CollisionLayer::Enemy, CollisionResponse::Block);

    // Ignore �ڱ� �� źȯ�� ��ü
    physicsManager.SetCollisionResponse(CollisionLayer::PlayerProjectile, CollisionLayer::Player, CollisionResponse::Ignore);
    physicsManager.SetCollisionResponse(CollisionLayer::EnemyProjectile, CollisionLayer::Enemy, CollisionResponse::Ignore);

    // Ignore źȯ���� �浹
    physicsManager.SetCollisionResponse(CollisionLayer::PlayerProjectile, CollisionLayer::EnemyProjectile, CollisionResponse::Ignore);
    physicsManager.SetCollisionResponse(CollisionLayer::PlayerProjectile, CollisionLayer::PlayerProjectile, CollisionResponse::Ignore);
    physicsManager.SetCollisionResponse(CollisionLayer::EnemyProjectile, CollisionLayer::EnemyProjectile, CollisionResponse::Ignore);

    // Default �� Projectile �浹�� ���ϸ� ����
    physicsManager.SetCollisionResponse(CollisionLayer::Default, CollisionLayer::PlayerProjectile, CollisionResponse::Block);
    physicsManager.SetCollisionResponse(CollisionLayer::Default, CollisionLayer::EnemyProjectile, CollisionResponse::Block);
}
