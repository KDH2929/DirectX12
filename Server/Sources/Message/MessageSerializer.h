#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include "Message/game_message.pb.h"  // game::GameMessage

// 고정 4바이트 길이 + GameMessage 직렬화/역직렬화
class MessageSerializer {
public:
    // 안전 한도
    static constexpr std::size_t MAX_MESSAGE_SIZE_BYTES = 1 * 1024 * 1024; // 1MB

    // 누적 버퍼에서 [len(4)][body] 하나를 꺼내 body 바이트를 반환
    // - 성공 시 input_buffer에서 소비된 만큼 지움
    // - NeedMoreData인 경우 false, 그 외 치명 오류는 error_message에 사유 기록
    static bool TryExtractMessage(std::string& input_buffer,
        std::string& output_body_bytes,
        std::string* error_message = nullptr);

    // GameMessage → body 바이트로 직렬화
    static bool Serialize(const game::GameMessage& message,
        std::string& out_body_bytes,
        std::string* error_message = nullptr);

    // body 바이트 → GameMessage 역직렬화
    static bool Deserialize(std::string_view body_bytes,
        game::GameMessage& out_message,
        std::string* error_message = nullptr);

    // [len(4)][body] 완성 (송신 직전 사용)
    static std::string BuildSerializedMessage(const game::GameMessage& message);

private:
    static bool ReadUint32LittleEndian(const char* data, std::size_t size, std::uint32_t& out_value);
    static void WriteUint32LittleEndian(char* data, std::uint32_t value);
};
