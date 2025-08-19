#pragma once

#include <mysql/mysql.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>

// Thread-safe MySQL connection pool (C API / libmysql)
class DatabaseConnectionPool {
public:
    DatabaseConnectionPool() = default;
    ~DatabaseConnectionPool();

    DatabaseConnectionPool(const DatabaseConnectionPool&) = delete;
    DatabaseConnectionPool& operator=(const DatabaseConnectionPool&) = delete;

    // ���̺귯�� �ʱ�ȭ(���μ����� 1ȸ) + Ŀ�ؼ� N�� ���Ҵ�
    bool Initialize(const std::string& host,
        unsigned int port,
        const std::string& user,
        const std::string& password,
        const std::string& database,
        unsigned int connectionCount);

    // ����� ������ �Ҹ��ڿ��� �ڵ����� ��ȯ�Ǵ� RAII ����
    class ConnectionGuard {
    public:
        ConnectionGuard(DatabaseConnectionPool* owner, MYSQL* connection)
            : owner(owner), connection(connection) {}
        ~ConnectionGuard();

        ConnectionGuard(const ConnectionGuard&) = delete;
        ConnectionGuard& operator=(const ConnectionGuard&) = delete;

        ConnectionGuard(ConnectionGuard&& other) noexcept
            : owner(other.owner), connection(other.connection) {
            other.owner = nullptr; other.connection = nullptr;
        }
        ConnectionGuard& operator=(ConnectionGuard&& other) noexcept {
            if (this != &other) {
                this->~ConnectionGuard();
                owner = other.owner; connection = other.connection;
                other.owner = nullptr; other.connection = nullptr;
            }
            return *this;
        }

        MYSQL* Get() const { return connection; }

    private:
        DatabaseConnectionPool* owner;
        MYSQL* connection;
    };

    // Ŀ�ؼ� 1�� �뿩(���ŷ)
    ConnectionGuard Acquire();

private:
    MYSQL* CreateConnection(const std::string& host,
        unsigned int port,
        const std::string& user,
        const std::string& password,
        const std::string& database);

    void Return(MYSQL* connection);

private:
    std::mutex poolMutex;
    std::condition_variable poolCondition;
    std::queue<MYSQL*> availableConnections;

    // ������� �ʿ��� ���� ����
    std::string host;
    unsigned int port = 0;
    std::string user;
    std::string password;
    std::string database;

    bool isInitialized = false;
};
