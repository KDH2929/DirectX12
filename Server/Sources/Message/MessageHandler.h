#pragma once
#include <memory>
#include "Message/game_message.pb.h"

class ClientSession;
class GameServer;

class MessageHandler {
public:
    explicit MessageHandler(GameServer* server) : gameServer(server) {}

    void HandleLogin(const std::shared_ptr<ClientSession>& session, const game::LoginRequest& request);
    void HandleJoinRoom(const std::shared_ptr<ClientSession>& session, const game::JoinRoomRequest& request);
    void HandleChatSend(const std::shared_ptr<ClientSession>& session, const game::ChatSend& request);
    void HandleMoveInput(const std::shared_ptr<ClientSession>& session, const game::MoveInput& request);
    void HandleFireInput(const std::shared_ptr<ClientSession>& session, const game::FireInput& request);
    void HandleHitEvent(const std::shared_ptr<ClientSession>& session, const game::HitEvent& request);

private:
    GameServer* gameServer;
};
