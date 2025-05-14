#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <string>
#include <thread>
#include <atomic>


class NetworkManager {
public:
    bool Connect(const std::string& ip, int port);
    void StartRecvThread();
    void Stop();
    void Send(const std::string& msg);

private:
    SOCKET sock = INVALID_SOCKET;
    std::thread recvThread;
    std::atomic<bool> running = false;

    void RecvLoop();
};
