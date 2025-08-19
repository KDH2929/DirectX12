#include <iostream>
#include <csignal>
#include <thread>
#include "GameServer.h"

namespace ServerRuntime {
    static std::atomic<bool> stopRequested{ false };
    static GameServer server;
}

// Ctrl+C 등 종료 시그널 처리
void SignalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n[INFO] 종료 요청 감지됨. 서버 종료 중...\n";
        ServerRuntime::stopRequested = true;
        ServerRuntime::server.Stop();
    }
}

int main() {
    const int port = 9999;

    std::signal(SIGINT, SignalHandler);

    if (!ServerRuntime::server.Start(port)) {
        std::cerr << "[ERROR] 서버 시작 실패\n";
        return 1;
    }

    std::cout << "[INFO] 서버가 실행 중입니다. Ctrl+C로 종료할 수 있습니다.\n";

    while (!ServerRuntime::stopRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[INFO] 서버가 정상적으로 종료되었습니다.\n";
    return 0;
}
