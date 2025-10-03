#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include "Message/game_message.pb.h"  // game::GameMessage

// ���� 4����Ʈ ���� + GameMessage ����ȭ/������ȭ
class MessageSerializer {
public:
    // ���� �ѵ�
    static constexpr std::size_t MAX_MESSAGE_SIZE_BYTES = 1 * 1024 * 1024; // 1MB

    // ���� ���ۿ��� [len(4)][body] �ϳ��� ���� body ����Ʈ�� ��ȯ
    // - ���� �� input_buffer���� �Һ�� ��ŭ ����
    // - NeedMoreData�� ��� false, �� �� ġ�� ������ error_message�� ���� ���
    static bool TryExtractMessage(std::string& input_buffer,
        std::string& output_body_bytes,
        std::string* error_message = nullptr);

    // GameMessage �� body ����Ʈ�� ����ȭ
    static bool Serialize(const game::GameMessage& message,
        std::string& out_body_bytes,
        std::string* error_message = nullptr);

    // body ����Ʈ �� GameMessage ������ȭ
    static bool Deserialize(std::string_view body_bytes,
        game::GameMessage& out_message,
        std::string* error_message = nullptr);

    // [len(4)][body] �ϼ� (�۽� ���� ���)
    static std::string BuildSerializedMessage(const game::GameMessage& message);

private:
    static bool ReadUint32LittleEndian(const char* data, std::size_t size, std::uint32_t& out_value);
    static void WriteUint32LittleEndian(char* data, std::uint32_t value);
};
