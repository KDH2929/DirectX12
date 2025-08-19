#include "ClientSession.h"
#include <cstring>
#include <iostream>
#include <chrono>

ClientSession::ClientSession(SOCKET socketHandleArg)
    : socketHandle(socketHandleArg) {}

ClientSession::~ClientSession() { Close(); }

void ClientSession::Close() {
    bool expected = false;
    if (!closed.compare_exchange_strong(expected, true)) return;

    if (socketHandle != INVALID_SOCKET) {
        shutdown(socketHandle, SD_BOTH);
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
    }
    std::lock_guard<std::mutex> lock(sendQueueMutex);
    sendQueue.clear();
}

void ClientSession::AppendToReceiveBuffer(const char* dataPointer, size_t sizeBytes) {
    receiveBuffer.append(dataPointer, sizeBytes);
}

bool ClientSession::TryExtractAndDispatchAllMessages(MessageDispatcher& dispatcher) {
    std::string body;
    game::GameMessage message;

    while (true) {
        std::string error;
        if (!MessageSerializer::TryExtractMessage(receiveBuffer, body, &error)) {
            if (!error.empty()) {
                std::cerr << "[Session] message extract error: " << error << "\n";
                return false;
            }
            return true; // NeedMoreData
        }
        if (!MessageSerializer::Deserialize(std::string_view(body), message, &error)) {
            std::cerr << "[Session] message deserialize error: " << error << "\n";
            return false;
        }
        dispatcher.Dispatch(shared_from_this(), message);
    }
}

void ClientSession::EnqueueSend(const std::string& bytes) {
    if (closed.load()) return;

    bool should_start = false;
    {
        std::lock_guard<std::mutex> lock(sendQueueMutex);
        should_start = sendQueue.empty();
        sendQueue.push_back(bytes);
    }
    if (should_start) TrySendNext();
}

void ClientSession::TrySendNext() {
    if (closed.load()) return;

    std::string nextBytes;
    {
        std::lock_guard<std::mutex> lock(sendQueueMutex);
        if (sendQueue.empty()) {
            sendingInProgress.store(false);
            return;
        }
        nextBytes = std::move(sendQueue.front());
        sendQueue.pop_front();
        sendingInProgress.store(true);
    }
    if (!SubmitSendOperation(nextBytes)) Close();
}

bool ClientSession::SubmitSendOperation(const std::string& bytes) {
    auto* ioContext = new IocpIoContext();
    ioContext->Reset(IoOperationType::Send, bytes.size());
    ioContext->ownerSession = shared_from_this();

    std::memcpy(ioContext->buffer.data(), bytes.data(), bytes.size());
    ioContext->wsaBuffer.buf = ioContext->buffer.data();
    ioContext->wsaBuffer.len = static_cast<ULONG>(bytes.size());

    DWORD bytesSent = 0;
    const int winSockResult = WSASend(
        socketHandle,
        &ioContext->wsaBuffer,
        1,
        &bytesSent,
        0,
        &ioContext->overlapped,
        nullptr
    );

    if (winSockResult == SOCKET_ERROR) {
        const int winSockErrorCode = WSAGetLastError();
        if (winSockErrorCode != WSA_IO_PENDING) {
            std::cerr << "[Session] WSASend failed. winSockErrorCode=" << winSockErrorCode << "\n";
            delete ioContext;
            return false;
        }
    }
    return true;
}

void ClientSession::SendGameMessage(const game::GameMessage& message) {
    EnqueueSend(MessageSerializer::BuildSerializedMessage(message));
}

void ClientSession::SendLoginResponse(game::StatusCode status_code,
    const std::string& status_message,
    std::uint32_t player_id_value,
    const std::string& session_token)
{
    if (closed.load()) return;

    game::GameMessage out;

    // Header
    auto* header = out.mutable_header();
    header->set_player_id(player_id_value);
    header->set_sequence(0);
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    header->set_server_time_ms(static_cast<std::uint64_t>(now_ms));

    // Payload: LoginResponse
    auto* loginResponse = out.mutable_login_response();
    auto* result = loginResponse->mutable_result();
    result->set_status_code(status_code);
    result->set_message(status_message);
    loginResponse->set_player_id(player_id_value);
    if (!session_token.empty()) loginResponse->set_session_token(session_token);

    SendGameMessage(out);
}
