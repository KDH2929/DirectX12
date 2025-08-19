#include <iostream>
#include <csignal>
#include <thread>
#include "GameServer.h"

namespace ServerRuntime {
    static std::atomic<bool> stopRequested{ false };
    static GameServer server;
}

// Ctrl+C �� ���� �ñ׳� ó��
void SignalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n[INFO] ���� ��û ������. ���� ���� ��...\n";
        ServerRuntime::stopRequested = true;
        ServerRuntime::server.Stop();
    }
}

int main() {
    const int port = 9999;

    std::signal(SIGINT, SignalHandler);

    if (!ServerRuntime::server.Start(port)) {
        std::cerr << "[ERROR] ���� ���� ����\n";
        return 1;
    }

    std::cout << "[INFO] ������ ���� ���Դϴ�. Ctrl+C�� ������ �� �ֽ��ϴ�.\n";

    while (!ServerRuntime::stopRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[INFO] ������ ���������� ����Ǿ����ϴ�.\n";
    return 0;
}
