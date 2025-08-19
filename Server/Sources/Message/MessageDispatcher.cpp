#include "MessageDispatcher.h"

MessageDispatcher::MessageDispatcher() = default;

MessageDispatcher::~MessageDispatcher() = default;

void MessageDispatcher::RegisterHandler(int payloadCase, std::function<void(std::shared_ptr<ClientSession>, const game::GameMessage&)> handlerFunction) {
    handlerMap[payloadCase] = std::move(handlerFunction);
}

void MessageDispatcher::Dispatch(std::shared_ptr<ClientSession> session, const game::GameMessage& message) const {
    int payloadCase = message.payload_case();  // oneof에서 payload_case 추출

    auto it = handlerMap.find(payloadCase);
    if (it != handlerMap.end()) {
        it->second(session, message);  // 해당 핸들러 호출
    }
    else {
        // 핸들러가 없다면 처리 안함 (혹은 에러 처리)
    }
}
