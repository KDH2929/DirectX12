#include "ClientSession.h"
#include <iostream>
#include <chrono>

ClientSession::ClientSession(SOCKET sock) : socket(sock) {}

ClientSession::~ClientSession() {
    StopSendThread();
}

SOCKET ClientSession::GetSocket() const {
    return socket;
}

void ClientSession::SendPacket(const game::Packet& packet) {
    std::string buffer;
    if (!packet.SerializeToString(&buffer)) {
        std::cerr << "[SendPacket] Failed to serialize packet\n";
        return;
    }
    EnqueuePacket(buffer);
}

void ClientSession::ApplyDamage(float dmg) {
    hp = std::max<float>(0.0f, hp - dmg);
}

bool ClientSession::IsAlive() const {
    return hp > 0;
}

void ClientSession::EnqueuePacket(const std::string& data) {
    std::lock_guard<std::mutex> lock(sendMutex);
    sendQueue.push(data);
}

void ClientSession::SendLoop() {
    using namespace std::chrono;
    while (sending) {
        std::string data;

        {
            std::lock_guard<std::mutex> lock(sendMutex);
            if (sendQueue.empty()) {
                std::this_thread::sleep_for(milliseconds(1));
                continue;
            }
            data = std::move(sendQueue.front());
            sendQueue.pop();
        }

        int result = send(socket, data.c_str(), static_cast<int>(data.size()), 0);
        if (result <= 0) {
            std::cerr << "[SendLoop] send() failed or client disconnected\n";
            break;
        }
    }

    std::cout << "[SendLoop] Ended for socket " << socket << "\n";
}

void ClientSession::StartSendThread() {
    sending = true;
    sendThread = std::thread(&ClientSession::SendLoop, this);
}

void ClientSession::StopSendThread() {
    sending = false;
    if (sendThread.joinable()) {
        sendThread.join();
    }
}
