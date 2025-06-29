#pragma once

#include <memory>

#include "ClientSession.h"
#include "packet.pb.h"


template <typename T>
game::Packet MakePacket(const std::string& typeName, const T& message) {
    std::string payload;
    message.SerializeToString(&payload);

    game::Packet packet;
    packet.set_type(typeName);
    packet.set_payload(payload);
    return packet;
}

class GameServer;       // 전방 선언

class MessageHandler {
public:

    static void SetGameServer(GameServer* server);

    static void HandleLogin(std::shared_ptr<ClientSession> client, const game::LoginRequest& request);
    static void HandleMove(std::shared_ptr<ClientSession> client, const game::Move& request);
    static void HandleChat(std::shared_ptr<ClientSession> client, const game::Chat& request);
    static void HandleFire(std::shared_ptr<ClientSession> client, const game::Fire& request);
    static void HandleHit(std::shared_ptr<ClientSession> client, const game::Hit& request);
    static void HandleJoinRoom(std::shared_ptr<ClientSession> client, const game::JoinRoomRequest& request);

private:
    static GameServer* gameServer;

};