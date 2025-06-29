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

    // �⺻ ����
    int playerId = -1;
    int roomId = -1;
    std::string nickname;
    Vector3 position;
    float hp = 100.0f;

    // ����
    void SendPacket(const game::Packet& packet);
    void ApplyDamage(float dmg);
    bool IsAlive() const;

    // �۽� ������ ����
    void StartSendThread();
    void StopSendThread();

private:
    SOCKET socket;

    // Send Queue ����
    std::mutex sendMutex;
    std::queue<std::string> sendQueue;
    std::atomic<bool> sending = true;
    std::thread sendThread;

    void SendLoop(); // ���� �۽� ����
    void EnqueuePacket(const std::string& data);
};
