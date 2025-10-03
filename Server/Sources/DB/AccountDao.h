#pragma once
#include <string>
#include <cstdint>

// ������ �ּ�ȭ�� ���� ���� ���� ���
struct MYSQL;

// Query �غ�(prepare) �� �Ķ���� ���ε�(bind) �� ����(execute)
// out �Ű������� �ش��ϴ� ����� DB�κ��� ��´�.

class AccountDao {
public:
    explicit AccountDao(MYSQL* connection) : connection(connection) {}

    // username�� �����ϸ� true, outAccountId / outPasswordHash ä��
    bool FindByUsername(const std::string& username,
        long long& outAccountId,
        std::string& outPasswordHash);

    // ���� ����, ������ id�� outAccountId�� ��ȯ
    bool CreateAccount(const std::string& username,
        const std::string& passwordHash,
        long long& outAccountId);

    // ������ �α��� �ð� ����
    bool UpdateLastLogin(long long accountId);

private:
    MYSQL* connection;
};
