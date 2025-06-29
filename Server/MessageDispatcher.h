#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include "packet.pb.h"
#include "ClientSession.h"

class MessageDispatcher {
public:
    using HandlerFunc = std::function<void(std::shared_ptr<ClientSession>, const game::Packet&)>;

    void Register(const std::string& messageType, HandlerFunc handler);
    void Dispatch(std::shared_ptr<ClientSession> client, const game::Packet& packet);

private:
    std::unordered_map<std::string, HandlerFunc> handlers;
};
