#pragma once
#include <unordered_set>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <cstdint>

class ClientSession;
struct Plane;

// ��������� ����� ������Ͽ��� �������� �ʱ� ���� ���� ����
namespace game { class GameMessage; }

class GameRoom {
public:
    explicit GameRoom(int roomId);

    void AddPlayer(ClientSession* clientSession);
    void RemovePlayer(ClientSession* clientSession);

    // �� ���� ��� �÷��̾�� GameMessage ��ε�ĳ��Ʈ
    void BroadcastGameMessage(const game::GameMessage& message,
        const ClientSession* excludedSession = nullptr);

    int GetRoomId() const { return roomId; }
    std::vector<ClientSession*> GetPlayerList() const;  // ��Ī �ܼ�ȭ
    int GetPlayerCount() const;

private:
    const int roomId;
    mutable std::mutex roomMutex;
    std::unordered_set<ClientSession*> players;  // ������ �ܺ�(Client �Ŵ���)�� ����
};
