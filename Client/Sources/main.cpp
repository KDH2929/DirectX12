#include "Game.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    Game game;

    if (!game.Init(hInstance, nCmdShow)) {
        MessageBox(nullptr, L"Failed to initialize the game", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    return game.Run();
}