#include "PacketParser.h"
#include <iostream>

bool PacketParser::Parse(const std::string& raw, game::Packet& outPacket) {
    return outPacket.ParseFromString(raw);
}