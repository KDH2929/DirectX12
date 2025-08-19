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

    // 라이브러리 초기화(프로세스당 1회) + 커넥션 N개 선할당
    bool Initialize(const std::string& host,
        unsigned int port,
        const std::string& user,
        const std::string& password,
        const std::string& database,
        unsigned int connectionCount);

    // 사용이 끝나면 소멸자에서 자동으로 반환되는 RAII 가드
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

    // 커넥션 1개 대여(블로킹)
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

    // 재생성에 필요한 접속 정보
    std::string host;
    unsigned int port = 0;
    std::string user;
    std::string password;
    std::string database;

    bool isInitialized = false;
};
