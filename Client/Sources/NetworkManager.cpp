#include "NetworkManager.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

bool NetworkManager::Connect(const std::string& ip, int port) {
    // Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[오류] WSAStartup 실패\n";
        return false;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[오류] 소켓 생성 실패\n";
        return false;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());  // 비추천된 방식이지만 일단 사용

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[오류] 서버 연결 실패\n";
        closesocket(sock);
        return false;
    }

    std::cout << "[성공] 서버에 연결됨\n";
    return true;
}

void NetworkManager::StartRecvThread() {
    running = true;
    recvThread = std::thread(&NetworkManager::RecvLoop, this);
}

void NetworkManager::RecvLoop() {
    char buffer[512];
    while (running) {
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cerr << "[서버 연결 끊김 or 오류]\n";
            running = false;
            break;
        }

        buffer[bytesReceived] = '\0';
        std::cout << "[서버] " << buffer << std::endl;
    }
}

void NetworkManager::Send(const std::string& msg) {
    if (sock != INVALID_SOCKET) {
        send(sock, msg.c_str(), static_cast<int>(msg.length()), 0);
    }
}

void NetworkManager::Stop() {
    running = false;

    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }

    if (recvThread.joinable()) {
        recvThread.join();
    }

    WSACleanup();
}
