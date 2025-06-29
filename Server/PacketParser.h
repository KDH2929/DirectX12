#pragma once
#include <string>
#include "packet.pb.h"

class PacketParser {
public:
    static bool Parse(const std::string& raw, game::Packet& outPacket);
};