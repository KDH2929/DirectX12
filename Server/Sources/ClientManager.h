#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <winsock2.h>
#include "ClientSession.h"

class ClientManager {
public:
    // Ŭ���̾�Ʈ �߰� �� �Ӽ� ����
    void AddClient(SOCKET socketHandle, int playerId = -1, const std::string& nickname = "");

    // ���� ���� ����
    void RemoveClient(SOCKET socketHandle);

    // ��ȸ (���� ����)
    std::shared_ptr<ClientSession> GetClientBySocket(SOCKET socketHandle) const;
    std::shared_ptr<ClientSession> GetClientByPlayerId(int playerId);

    // ��� ��ȸ (���纻 ��ȯ)
    // ��� Ŭ���̾�Ʈ ��� ��ȯ
    std::vector<std::shared_ptr<ClientSession>> GetAllClientList() const;
    // Ư�� �� Ŭ���̾�Ʈ ��� ��ȯ
    std::vector<std::shared_ptr<ClientSession>> GetClientsInRoomList(int roomId) const;

private:
    mutable std::mutex managerMutex;

    // �⺻ �����: ���� �� ����
    std::unordered_map<SOCKET, std::shared_ptr<ClientSession>> sessionsBySocket;

    // ���� �ε���: playerId �� ����(weak, ������ �⺻ ����Ұ� ����)
    std::unordered_map<int, std::weak_ptr<ClientSession>> sessionsByPlayerId;

    // ����: ���� �ε������� ����� �׸� ����
    void PruneExpiredPlayerIndexLocked();
};
