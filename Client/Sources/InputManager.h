#pragma once
#include <windows.h>
#include <DirectXMath.h>

using namespace DirectX;

enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2
};

class InputManager {
public:
    static InputManager& GetInstance() {
        static InputManager instance;
        return instance;
    }

    void Initialize(HWND hwnd);

    // 키입력 관련
    void OnKeyDown(UINT nChar);
    void OnKeyUp(UINT nChar);

    bool IsKeyHeld(UINT nChar) const;
    bool IsKeyJustPressed(UINT nChar) const;

    // 좌우키에 대해 double press 여부 반환
    bool IsDoublePressedLeft() const;
    bool IsDoublePressedRight() const;

    // 마우스 입력 처리
    void OnMouseMove(int x, int y);
    void OnMouseDown(MouseButton button);   // button: 0-좌클릭, 1-우클릭, 2-휠
    void OnMouseUp(MouseButton button);

    void OnRawMouseMove(int dx, int dy);

    // 마우스 상태 확인 메서드
    int GetMouseX() const;
    int GetMouseY() const;
    bool IsMouseButtonDown(MouseButton button) const;
    bool IsMouseButtonUp(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;       // "눌렀다 떼었는지" 확인하는 함수

    int GetMouseDeltaX() const;
    int GetMouseDeltaY() const;

    XMFLOAT2 GetMouseDirectionFromCenter();

    void SetCursorHidden(bool hide);
    void SetCursorCentor();

    void Update(float deltaTime);

private:
    InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    HWND hwnd = nullptr;

    // 키 입력 상태
    bool keyStates[256] = { false };
    bool prevKeyStates[256] = { false };

    // 마우스 입력 상태
    int mouseX = 0;
    int mouseY = 0;
    bool mouseButtonStates[3] = { false, false, false };
    bool prevMouseButtonStates[3] = { false, false, false };

    int mouseDeltaX = 0;
    int mouseDeltaY = 0;

    // 마우스 이동감지를 위한 Raw Input 관련 변수
    int rawMouseDeltaX = 0;
    int rawMouseDeltaY = 0;

    bool lockMouseToCenter = false;
    bool ignoreNextMouseDelta = false;


    // 좌우 키에 대한 double press 타이머 (ms 단위)
    float leftKeyTimer = 0.0f;
    float rightKeyTimer = 0.0f;


    bool leftKeyComboInputActive = false;
    bool rightKeyComboInputActive = false;

    bool leftKeyDoublePressed = false;
    bool rightKeyDoublePressed = false;


    // 연속입력 임계치 (ms 단위)
    static constexpr float DOUBLE_PRESS_THRESHOLD = 200.0f;
};
