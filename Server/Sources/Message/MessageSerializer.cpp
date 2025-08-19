#include "MessageSerializer.h"
#include <cstring>

bool MessageSerializer::ReadUint32LittleEndian(const char* data, std::size_t size, std::uint32_t& out_value) {
    if (size < 4) return false;
    std::memcpy(&out_value, data, 4); // Windows 리틀엔디언 가정
    return true;
}

void MessageSerializer::WriteUint32LittleEndian(char* data, std::uint32_t value) {
    std::memcpy(data, &value, 4);
}

bool MessageSerializer::TryExtractMessage(std::string& input_buffer,
    std::string& output_body_bytes,
    std::string* error_message)
{
    // NeedMoreData: 길이 헤더 부족
    if (input_buffer.size() < 4) {
        if (error_message) error_message->clear();
        return false;
    }

    std::uint32_t body_length = 0;
    if (!ReadUint32LittleEndian(input_buffer.data(), input_buffer.size(), body_length)) {
        if (error_message) *error_message = "malformed length header";
        return false;
    }

    if (body_length > MAX_MESSAGE_SIZE_BYTES) {
        if (error_message) *error_message = "message too large";
        return false;
    }

    const std::size_t total = 4u + static_cast<std::size_t>(body_length);
    // NeedMoreData: 바디 부족
    if (input_buffer.size() < total) {
        if (error_message) error_message->clear();
        return false;
    }

    output_body_bytes.assign(input_buffer.data() + 4, body_length);
    input_buffer.erase(0, total);
    if (error_message) error_message->clear();
    return true;
}

bool MessageSerializer::Serialize(const game::GameMessage& message,
    std::string& out_body_bytes,
    std::string* error_message)
{
    out_body_bytes.clear();
    out_body_bytes.reserve(message.ByteSizeLong());
    const bool ok = message.SerializeToString(&out_body_bytes);
    if (!ok && error_message) *error_message = "protobuf serialize failed";
    return ok;
}

bool MessageSerializer::Deserialize(std::string_view body_bytes,
    game::GameMessage& out_message,
    std::string* error_message)
{
    if (body_bytes.size() > MAX_MESSAGE_SIZE_BYTES) {
        if (error_message) *error_message = "message too large";
        return false;
    }

    const bool ok = out_message.ParseFromArray(body_bytes.data(),
        static_cast<int>(body_bytes.size()));
    if (!ok) {
        if (error_message) *error_message = "protobuf parse error";
        return false;
    }

    if (out_message.payload_case() == game::GameMessage::PAYLOAD_NOT_SET) {
        if (error_message) *error_message = "empty payload (oneof not set)";
        return false;
    }

    if (error_message) error_message->clear();
    return true;
}

std::string MessageSerializer::BuildSerializedMessage(const game::GameMessage& message)
{
    std::string body;
    body.reserve(message.ByteSizeLong());
    message.SerializeToString(&body);

    const std::uint32_t body_len = static_cast<std::uint32_t>(body.size());
    std::string out;
    out.resize(4 + body_len);
    WriteUint32LittleEndian(out.data(), body_len);
    std::memcpy(out.data() + 4, body.data(), body_len);
    return out;
}
