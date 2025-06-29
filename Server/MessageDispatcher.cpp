#include "MessageDispatcher.h"
#include <iostream>

void MessageDispatcher::Register(const std::string& messageType, HandlerFunc handler) {
    handlers[messageType] = handler;
}

void MessageDispatcher::Dispatch(std::shared_ptr<ClientSession> client, const game::Packet& packet) {
    auto it = handlers.find(packet.type());
    if (it != handlers.end()) {
        it->second(client, packet);
    }
    else {
        std::cerr << "[Dispatcher] Unknown packet type: " << packet.type() << "\n";
    }
}
