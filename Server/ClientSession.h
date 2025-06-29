#pragma once
#include <winsock2.h>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include "packet.pb.h"

struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
};

class ClientSession {
public:
    explicit ClientSession(SOCKET socket);
    ~ClientSession();

    SOCKET GetSocket() const;

    // 기본 정보
    int playerId = -1;
    int roomId = -1;
    std::string nickname;
    Vector3 position;
    float hp = 100.0f;

    // 동작
    void SendPacket(const game::Packet& packet);
    void ApplyDamage(float dmg);
    bool IsAlive() const;

    // 송신 스레드 관리
    void StartSendThread();
    void StopSendThread();

private:
    SOCKET socket;

    // Send Queue 관련
    std::mutex sendMutex;
    std::queue<std::string> sendQueue;
    std::atomic<bool> sending = true;
    std::thread sendThread;

    void SendLoop(); // 내부 송신 루프
    void EnqueuePacket(const std::string& data);
};
