#include "Game.h"

#include <pix3.h>


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    Game game;

    PIXLoadLatestWinPixGpuCapturerLibrary();

    if (!game.Init(hInstance, nCmdShow)) {
        MessageBox(nullptr, L"Failed to initialize the game", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    return game.Run();
}