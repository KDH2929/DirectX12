#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include "game_message.pb.h"  // game::GameMessage

// ���� ����
class ClientSession;

class MessageDispatcher {
public:
    MessageDispatcher();
    ~MessageDispatcher();

    MessageDispatcher(const MessageDispatcher&) = delete;
    MessageDispatcher& operator=(const MessageDispatcher&) = delete;

    // �ڵ鷯 ���: �޽��� Ÿ�Կ� ���� �Լ� ���
    void RegisterHandler(int payloadCase, std::function<void(std::shared_ptr<ClientSession>, const game::GameMessage&)> handlerFunction);

    // ����ġ: ��ϵ� �ڵ鷯�� ȣ��
    void Dispatch(std::shared_ptr<ClientSession> session, const game::GameMessage& message) const;

private:
    // payloadCase�� ���� �ڵ鷯 �Լ� ��
    std::unordered_map<int, std::function<void(std::shared_ptr<ClientSession>, const game::GameMessage&)>> handlerMap;
};
