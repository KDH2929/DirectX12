#include <iostream>
#include <csignal>      // Ctrl+C 핸들링용
#include "GameServer.h"

GameServer server;
bool stopRequested = false;

// Ctrl+C 등 종료 시그널 처리
void SignalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n[INFO] 종료 요청 감지됨. 서버 종료 중...\n";
        stopRequested = true;
        server.Stop();
    }
}

int main() {
    const int port = 9999;

    // 시그널 핸들러 등록
    std::signal(SIGINT, SignalHandler);

    if (!server.Start(port)) {
        std::cerr << "[ERROR] 서버 시작 실패\n";
        return 1;
    }

    std::cout << "[INFO] 서버가 실행 중입니다. Ctrl+C로 종료할 수 있습니다.\n";

    // 메인 스레드는 기다리기만 함 (서브 스레드가 서버 실행)
    while (!stopRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[INFO] 서버가 정상적으로 종료되었습니다.\n";
    return 0;
}
