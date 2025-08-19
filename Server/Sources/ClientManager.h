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
    // 클라이언트 추가 및 속성 설정
    void AddClient(SOCKET socketHandle, int playerId = -1, const std::string& nickname = "");

    // 소켓 기준 제거
    void RemoveClient(SOCKET socketHandle);

    // 조회 (개별 세션)
    std::shared_ptr<ClientSession> GetClientBySocket(SOCKET socketHandle) const;
    std::shared_ptr<ClientSession> GetClientByPlayerId(int playerId);

    // 목록 조회 (복사본 반환)
    // 모든 클라이언트 목록 반환
    std::vector<std::shared_ptr<ClientSession>> GetAllClientList() const;
    // 특정 방 클라이언트 목록 반환
    std::vector<std::shared_ptr<ClientSession>> GetClientsInRoomList(int roomId) const;

private:
    mutable std::mutex managerMutex;

    // 기본 저장소: 소켓 → 세션
    std::unordered_map<SOCKET, std::shared_ptr<ClientSession>> sessionsBySocket;

    // 보조 인덱스: playerId → 세션(weak, 수명은 기본 저장소가 관리)
    std::unordered_map<int, std::weak_ptr<ClientSession>> sessionsByPlayerId;

    // 헬퍼: 보조 인덱스에서 만료된 항목 정리
    void PruneExpiredPlayerIndexLocked();
};
