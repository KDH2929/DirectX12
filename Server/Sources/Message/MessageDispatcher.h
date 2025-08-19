#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include "game_message.pb.h"  // game::GameMessage

// 전방 선언
class ClientSession;

class MessageDispatcher {
public:
    MessageDispatcher();
    ~MessageDispatcher();

    MessageDispatcher(const MessageDispatcher&) = delete;
    MessageDispatcher& operator=(const MessageDispatcher&) = delete;

    // 핸들러 등록: 메시지 타입에 따라 함수 등록
    void RegisterHandler(int payloadCase, std::function<void(std::shared_ptr<ClientSession>, const game::GameMessage&)> handlerFunction);

    // 디스패치: 등록된 핸들러를 호출
    void Dispatch(std::shared_ptr<ClientSession> session, const game::GameMessage& message) const;

private:
    // payloadCase에 따른 핸들러 함수 맵
    std::unordered_map<int, std::function<void(std::shared_ptr<ClientSession>, const game::GameMessage&)>> handlerMap;
};
