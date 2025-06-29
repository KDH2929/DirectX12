#include "MessageHandler.h"
#include "GameServer.h"
#include <iostream>

GameServer* MessageHandler::gameServer = nullptr;

void MessageHandler::SetGameServer(GameServer* server)
{
    gameServer = server;
}

void MessageHandler::HandleLogin(std::shared_ptr<ClientSession> client, const game::LoginRequest& request) {
    std::cout << "[Login] Username: " << request.username() << "\n";

    client->nickname = request.username();

    game::LoginResponse response;
    response.set_success(true);
    response.set_message("Welcome, " + request.username() + "!");

    auto packet = MakePacket("LoginResponse", response);
    client->SendPacket(packet);
}

void MessageHandler::HandleMove(std::shared_ptr<ClientSession> client, const game::Move& request) {
    client->position = { request.x(), request.y(), request.z() };

    GameRoom* room = gameServer->GetRoomManager()->GetRoom(client->roomId);
    if (!room) return;

    game::GameState state;
    for (auto player : room->GetPlayers()) {
        game::PlayerInfo* info = state.add_players();
        info->set_player_id(player->playerId);
        info->set_nickname(player->nickname);
        info->set_x(player->position.x);
        info->set_y(player->position.y);
        info->set_z(player->position.z);
        info->set_hp(player->hp);
    }

    game::Packet packet = MakePacket("GameState", state);
    room->Broadcast(packet, nullptr);
}

void MessageHandler::HandleChat(std::shared_ptr<ClientSession> client, const game::Chat& request) {
    std::cout << "[Chat] " << client->nickname << ": " << request.message() << "\n";

    game::Chat response;
    response.set_nickname(client->nickname);
    response.set_message(request.message());

    auto packet = MakePacket("ChatBroadcast", response);

    auto* room = gameServer->GetRoomManager()->GetRoom(client->roomId);
    if (room) {
        room->Broadcast(packet, nullptr);    // 모두에게 채팅 브로드캐스트
    }
}

void MessageHandler::HandleFire(std::shared_ptr<ClientSession> client, const game::Fire& request) {
    game::Fire response;
    response.set_bullet_id(request.bullet_id());
    response.set_dir_x(request.dir_x());
    response.set_dir_y(request.dir_y());
    response.set_dir_z(request.dir_z());
    response.set_player_id(client->playerId); // 추가

    auto packet = MakePacket("FireBroadcast", response);
    gameServer->GetRoomManager()->BroadcastToRoom(client->roomId, packet, client.get());
}

void MessageHandler::HandleHit(std::shared_ptr<ClientSession> client, const game::Hit& request) {
    auto* room = gameServer->GetRoomManager()->GetRoom(client->roomId);
    if (!room) return;

    for (auto* player : room->GetPlayers()) {
        if (player && player->playerId == request.target_id()) {
            player->ApplyDamage(request.damage());

            game::Hit hitResponse;
            hitResponse.set_attacker_id(request.attacker_id());
            hitResponse.set_target_id(request.target_id());
            hitResponse.set_damage(request.damage());

            auto packet = MakePacket("HitBroadcast", hitResponse);
            room->Broadcast(packet);
            break;
        }
    }
}
void MessageHandler::HandleJoinRoom(std::shared_ptr<ClientSession> client, const game::JoinRoomRequest& request) {
    int roomId = request.room_id();
    client->roomId = roomId;

    std::cout << "[JoinRoom] Player " << client->nickname << " joined room " << roomId << "\n";

    GameRoom& room = gameServer->GetRoomManager()->GetOrCreateRoom(roomId);
    room.AddPlayer(client.get());

    game::JoinRoomResponse response;
    response.set_success(true);
    response.set_message("Joined room " + std::to_string(roomId));

    auto packet = MakePacket("JoinRoomResponse", response);
    client->SendPacket(packet);
}