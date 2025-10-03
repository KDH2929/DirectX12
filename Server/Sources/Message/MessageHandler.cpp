#include "MessageHandler.h"
#include "ClientSession.h"
#include "GameServer.h"
#include "RoomManager.h"
#include "GameRoom.h"
#include "Message/game_message.pb.h"

#include <iostream>

static inline void FillMessageHeader(game::GameMessage& message,
    std::uint32_t playerId,
    std::uint64_t sequence,
    std::uint64_t serverTimeMilliseconds)
{
    game::Header* header = message.mutable_header();
    header->set_player_id(playerId);
    if (sequence != 0) header->set_sequence(sequence);
    header->set_server_time_ms(serverTimeMilliseconds);
}

// �α��� ó��
void MessageHandler::HandleLogin(const std::shared_ptr<ClientSession>& session,
    const game::LoginRequest& request)
{
    if (request.username().empty()) {
        // �߸��� ��û (�ʵ� ����)
        game::LoginResponse response;
        auto* result = response.mutable_result();
        result->set_status_code(game::StatusCode::BAD_REQUEST);
        result->set_message("username is empty");

        game::GameMessage message;
        *message.mutable_login_response() = response;
        FillMessageHeader(message, 0, 0, gameServer->NowMillis());

        session->SendGameMessage(message);
        return;
    }

    // �ܼ� �α��� ó��: ����ڸ� ���
    session->nickname = request.username();

    game::LoginResponse response;
    auto* result = response.mutable_result();
    result->set_status_code(game::StatusCode::SUCCESS);
    result->set_message("OK");
    response.set_player_id(static_cast<std::uint32_t>(session->playerId));
    response.set_session_token("dummy-token");

    game::GameMessage message;
    *message.mutable_login_response() = response;
    FillMessageHeader(message,
        static_cast<std::uint32_t>(session->playerId),
        0,
        gameServer->NowMillis());

    session->SendGameMessage(message);
    std::cout << "[Login] username=" << request.username() << " ok\n";
}

// �� ���� ó��
void MessageHandler::HandleJoinRoom(const std::shared_ptr<ClientSession>& session,
    const game::JoinRoomRequest& request)
{
    RoomManager* roomManager = gameServer->GetRoomManager();
    GameRoom& room = roomManager->GetOrCreateRoom(static_cast<int>(request.room_id()));

    room.AddPlayer(session.get());
    session->roomId = static_cast<int>(request.room_id());

    game::JoinRoomResponse response;
    auto* result = response.mutable_result();
    result->set_status_code(game::StatusCode::SUCCESS);
    result->set_message("OK");
    response.set_room_id(request.room_id());

    // �� �÷��̾� ��� ä��� (player_list)
    for (ClientSession* player : room.GetPlayerList()) {
        if (!player) continue;
        game::PlayerInfo* info = response.add_player_list();
        info->set_player_id(static_cast<std::uint32_t>(player->playerId));
        info->set_nickname(player->nickname);
        info->mutable_position()->set_x(player->position.x);
        info->mutable_position()->set_y(player->position.y);
        info->mutable_position()->set_z(player->position.z);
        info->set_hp(static_cast<std::uint32_t>(player->healthPoints));
    }

    game::GameMessage message;
    *message.mutable_join_room_response() = response;
    FillMessageHeader(message,
        static_cast<std::uint32_t>(session->playerId),
        0,
        gameServer->NowMillis());

    session->SendGameMessage(message);
    std::cout << "[JoinRoom] player=" << session->nickname
        << " room=" << request.room_id() << "\n";
}

// ä�� ó��
void MessageHandler::HandleChatSend(const std::shared_ptr<ClientSession>& session,
    const game::ChatSend& request)
{
    GameRoom* room = gameServer->GetRoomManager()->GetRoom(session->roomId);
    if (!room) return;

    game::ChatEvent eventMessage;
    eventMessage.set_player_id(static_cast<std::uint32_t>(session->playerId));
    eventMessage.set_nickname(session->nickname);
    eventMessage.set_message(request.message());
    eventMessage.set_server_time_ms(gameServer->NowMillis());

    game::GameMessage message;
    *message.mutable_chat_event() = eventMessage;
    FillMessageHeader(message,
        static_cast<std::uint32_t>(session->playerId),
        0,
        eventMessage.server_time_ms());

    room->BroadcastGameMessage(message, nullptr);
}

// �̵� �Է� ó��
void MessageHandler::HandleMoveInput(const std::shared_ptr<ClientSession>& session,
    const game::MoveInput& request)
{
    GameRoom* room = gameServer->GetRoomManager()->GetRoom(session->roomId);
    if (!room) return;

    // �ܼ� ����: Ŭ���̾�Ʈ�� ���� ��ġ�� �״�� ����
    session->position = { request.position().x(),
                          request.position().y(),
                          request.position().z() };

    game::MoveEvent eventMessage;
    eventMessage.set_player_id(static_cast<std::uint32_t>(session->playerId));
    eventMessage.mutable_position()->set_x(session->position.x);
    eventMessage.mutable_position()->set_y(session->position.y);
    eventMessage.mutable_position()->set_z(session->position.z);
    eventMessage.mutable_velocity()->CopyFrom(request.direction());
    eventMessage.set_server_time_ms(gameServer->NowMillis());

    game::GameMessage message;
    *message.mutable_move_event() = eventMessage;
    FillMessageHeader(message,
        static_cast<std::uint32_t>(session->playerId),
        request.tick(),
        eventMessage.server_time_ms());

    room->BroadcastGameMessage(message, nullptr);
}

// �߻� �Է� ó��
void MessageHandler::HandleFireInput(const std::shared_ptr<ClientSession>& session,
    const game::FireInput& request)
{
    GameRoom* room = gameServer->GetRoomManager()->GetRoom(session->roomId);
    if (!room) return;

    game::FireEvent eventMessage;
    eventMessage.set_player_id(static_cast<std::uint32_t>(session->playerId));
    eventMessage.mutable_origin()->CopyFrom(request.origin());
    eventMessage.mutable_direction()->CopyFrom(request.direction());
    eventMessage.set_bullet_id(request.weapon_id()); // �ӽ�: ���� �߱����� ���� ����
    eventMessage.set_server_time_ms(gameServer->NowMillis());

    game::GameMessage message;
    *message.mutable_fire_event() = eventMessage;
    FillMessageHeader(message,
        static_cast<std::uint32_t>(session->playerId),
        request.tick(),
        eventMessage.server_time_ms());

    room->BroadcastGameMessage(message, nullptr);
}

// �ǰ� �̺�Ʈ ó��
void MessageHandler::HandleHitEvent(const std::shared_ptr<ClientSession>& session,
    const game::HitEvent& request)
{
    GameRoom* room = gameServer->GetRoomManager()->GetRoom(session->roomId);
    if (!room) return;

    for (ClientSession* player : room->GetPlayerList()) {
        if (!player) continue;
        if (static_cast<std::uint32_t>(player->playerId) != request.target_id()) continue;

        // �ܼ� ���� ����: ������ ����
        player->ApplyDamage(static_cast<float>(request.damage()));

        game::HitEvent eventMessage = request; // ���� �� ü��/�ð��� ����
        eventMessage.set_target_hp(static_cast<std::uint32_t>(player->healthPoints));
        eventMessage.set_server_time_ms(gameServer->NowMillis());

        game::GameMessage message;
        *message.mutable_hit_event() = eventMessage;
        FillMessageHeader(message,
            static_cast<std::uint32_t>(session->playerId),
            0,
            eventMessage.server_time_ms());

        room->BroadcastGameMessage(message, nullptr);
        break;
    }
}
