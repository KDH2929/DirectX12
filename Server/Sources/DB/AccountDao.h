#pragma once
#include <string>
#include <cstdint>

// 의존성 최소화를 위해 전방 선언만 사용
struct MYSQL;

// Query 준비(prepare) → 파라미터 바인딩(bind) → 실행(execute)
// out 매개변수에 해당하는 결과를 DB로부터 얻는다.

class AccountDao {
public:
    explicit AccountDao(MYSQL* connection) : connection(connection) {}

    // username이 존재하면 true, outAccountId / outPasswordHash 채움
    bool FindByUsername(const std::string& username,
        long long& outAccountId,
        std::string& outPasswordHash);

    // 계정 생성, 생성된 id를 outAccountId로 반환
    bool CreateAccount(const std::string& username,
        const std::string& passwordHash,
        long long& outAccountId);

    // 마지막 로그인 시각 갱신
    bool UpdateLastLogin(long long accountId);

private:
    MYSQL* connection;
};
