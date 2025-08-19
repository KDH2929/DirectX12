#include "InputManager.h"
#include <Windows.h>
#include <algorithm>

void InputManager::Initialize(HWND hwnd)
{
    this->hwnd = hwnd;

    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01; // 일반 데스크탑
    rid.usUsage = 0x02;     // 마우스
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;

    RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

void InputManager::OnKeyDown(UINT nChar) {
    if (nChar < 256)
        keyStates[nChar] = true;
}

void InputManager::OnKeyUp(UINT nChar) {
    if (nChar < 256)
        keyStates[nChar] = false;
}
bool InputManager::IsKeyHeld(UINT nChar) const {
    return (nChar < 256) ? keyStates[nChar] : false;
}

bool InputManager::IsKeyJustPressed(UINT nChar) const {
    if (nChar >= 256)
        return false;

    // 새롭게 눌린지 파악
    return keyStates[nChar] && !prevKeyStates[nChar];
}

void InputManager::Update(float deltaTime) {

    // 다음 프레임에서 초기화처리
    // 현재 캐릭터 Update -> InputMangaer Update 순으로 처리됨을 유의하면 됨

    mouseDeltaX = rawMouseDeltaX;
    mouseDeltaY = rawMouseDeltaY;

    rawMouseDeltaX = 0;
    rawMouseDeltaY = 0;


    leftKeyDoublePressed = false;
    rightKeyDoublePressed = false;


    // 좌측 키(VK_LEFT) 연속 입력 확인
    if (IsKeyJustPressed(VK_LEFT)) {

        if (leftKeyComboInputActive && leftKeyTimer < DOUBLE_PRESS_THRESHOLD)
        {
            leftKeyComboInputActive = false;
            leftKeyDoublePressed = true;
        }

        else
        {
            leftKeyDoublePressed = false;
        }

        leftKeyComboInputActive = true;
        leftKeyTimer = 0.0f;
    }


    else {
        leftKeyTimer += deltaTime;

        if (leftKeyTimer >= DOUBLE_PRESS_THRESHOLD)
        {
            leftKeyComboInputActive = false;
            leftKeyDoublePressed = false;
        }
    }


    // 우측 키(VK_RIGHT) 연속 입력 확인
    if (IsKeyJustPressed(VK_RIGHT)) {

        if (rightKeyComboInputActive && rightKeyTimer < DOUBLE_PRESS_THRESHOLD)
        {
            rightKeyComboInputActive = false;
            rightKeyDoublePressed = true;
        }

        else
        {
            rightKeyDoublePressed = false;
        }

        rightKeyComboInputActive = true;
        rightKeyTimer = 0.0f;
    }


    else {
        rightKeyTimer += deltaTime;

        if (rightKeyTimer >= DOUBLE_PRESS_THRESHOLD)
        {
            rightKeyComboInputActive = false;
            rightKeyDoublePressed = false;
        }
    }




    // 이전 프레임의 키 상태 업데이트
    for (int i = 0; i < 256; ++i)
    {
        prevKeyStates[i] = keyStates[i];
    }

    // 이전 프레임의 마우스 상태 저장
    for (int i = 0; i < 3; ++i)
    {
        prevMouseButtonStates[i] = mouseButtonStates[i];
    }

}


bool InputManager::IsDoublePressedLeft() const {
    return leftKeyDoublePressed;
}

bool InputManager::IsDoublePressedRight() const {
    return rightKeyDoublePressed;
}

void InputManager::OnMouseMove(int x, int y)
{
    mouseX = x;
    mouseY = y;
}

void InputManager::OnMouseDown(MouseButton button)
{
    int index = static_cast<int>(button);
    if (index < 3) {
        mouseButtonStates[index] = true;
    }
}

void InputManager::OnMouseUp(MouseButton button)
{
    int index = static_cast<int>(button);
    if (index < 3) {
        mouseButtonStates[index] = false;
    }
}

void InputManager::OnRawMouseMove(int dx, int dy)
{
    rawMouseDeltaX += dx;
    rawMouseDeltaY += dy;
}

int InputManager::GetMouseX() const
{
    return mouseX;
}

int InputManager::GetMouseY() const {
    return mouseY;
}

bool InputManager::IsMouseButtonDown(MouseButton button) const {
    int index = static_cast<int>(button);
    return (index < 3) ? mouseButtonStates[index] : false;
}

bool InputManager::IsMouseButtonUp(MouseButton button) const
{
    int index = static_cast<int>(button);
    return (index < 3) ? !mouseButtonStates[index] : false;
}

bool InputManager::IsMouseButtonReleased(MouseButton button) const
{
    int index = static_cast<int>(button);
    return (index < 3) ? (!mouseButtonStates[index] && prevMouseButtonStates[index]) : false;
}

int InputManager::GetMouseDeltaX() const
{
    return mouseDeltaX;
}

int InputManager::GetMouseDeltaY() const
{
    return mouseDeltaY;
}

XMFLOAT2 InputManager::GetMouseDirectionFromCenter()
{
    if (!hwnd) return XMFLOAT2(0, 0);

    RECT rect;
    GetClientRect(hwnd, &rect);
    int screenWidth = rect.right - rect.left;
    int screenHeight = rect.bottom - rect.top;

    POINT mousePos = { 0 };
    GetCursorPos(&mousePos);               // 마우스 위치를 스크린 좌표 기준으로
    ScreenToClient(hwnd, &mousePos);       // → 클라이언트(윈도우 내부) 좌표로 변환

    float centerX = screenWidth * 0.5f;
    float centerY = screenHeight * 0.5f;

    float normX = (mousePos.x - centerX) / centerX;
    float normY = (centerY - mousePos.y) / centerY;  // Y 반전

    // 클램핑 (최대 범위 초과 방지)
    normX = std::clamp(normX, -1.0f, 1.0f);
    normY = std::clamp(normY, -1.0f, 1.0f);

    return XMFLOAT2(normX, normY);
}

void InputManager::SetCursorHidden(bool hide)
{
    ShowCursor(!hide);
}

void InputManager::SetCursorCentor()
{
    if (!hwnd) return;

    RECT rect;
    GetClientRect(hwnd, &rect);
    POINT center;
    center.x = (rect.right - rect.left) / 2;
    center.y = (rect.bottom - rect.top) / 2;

    ClientToScreen(hwnd, &center);
    SetCursorPos(center.x, center.y);
}
