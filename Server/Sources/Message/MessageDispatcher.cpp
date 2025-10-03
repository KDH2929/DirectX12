#include "MessageDispatcher.h"

MessageDispatcher::MessageDispatcher() = default;

MessageDispatcher::~MessageDispatcher() = default;

void MessageDispatcher::RegisterHandler(int payloadCase, std::function<void(std::shared_ptr<ClientSession>, const game::GameMessage&)> handlerFunction) {
    handlerMap[payloadCase] = std::move(handlerFunction);
}

void MessageDispatcher::Dispatch(std::shared_ptr<ClientSession> session, const game::GameMessage& message) const {
    int payloadCase = message.payload_case();  // oneof���� payload_case ����

    auto it = handlerMap.find(payloadCase);
    if (it != handlerMap.end()) {
        it->second(session, message);  // �ش� �ڵ鷯 ȣ��
    }
    else {
        // �ڵ鷯�� ���ٸ� ó�� ���� (Ȥ�� ���� ó��)
    }
}
