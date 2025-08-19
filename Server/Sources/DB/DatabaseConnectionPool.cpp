#include "DatabaseConnectionPool.h"
#include <mutex>

static void EnsureMysqlLibraryInitializedOnce()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() {
        // 실패 시 예외를 던지지 않고, Initialize 쪽에서 false를 반환하게 하려면
        // 상태를 외부로 전달해야 하지만, libmysql은 0이면 성공이라 여기서 단순 호출.
        mysql_library_init(0, nullptr, nullptr);
        });
}

DatabaseConnectionPool::~DatabaseConnectionPool()
{
    // 큐에 남은 연결 정리
    while (!availableConnections.empty()) {
        MYSQL* connection = availableConnections.front();
        availableConnections.pop();
        if (connection) mysql_close(connection);
    }
    // mysql_library_end()는 프로세스 종료 시점에 호출해도 되고 생략해도 됨(전역 사용 시 꼬임 방지용으로 생략)
}

bool DatabaseConnectionPool::Initialize(const std::string& host_,
    unsigned int port_,
    const std::string& user_,
    const std::string& password_,
    const std::string& database_,
    unsigned int connectionCount)
{
    EnsureMysqlLibraryInitializedOnce();

    host = host_;
    port = port_;
    user = user_;
    password = password_;
    database = database_;

    // 미리 N개 생성
    for (unsigned int i = 0; i < connectionCount; ++i) {
        MYSQL* connection = CreateConnection(host, port, user, password, database);
        if (!connection) {
            return false;
        }
        availableConnections.push(connection);
    }

    isInitialized = true;
    return true;
}

MYSQL* DatabaseConnectionPool::CreateConnection(const std::string& host_,
    unsigned int port_,
    const std::string& user_,
    const std::string& password_,
    const std::string& database_)
{
    MYSQL* connection = mysql_init(nullptr);
    if (!connection) return nullptr;

    // 자동 재연결
#if defined(MYSQL_VERSION_ID) && (MYSQL_VERSION_ID >= 80000)
    bool reconnect = true;
    mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect);
#else
    my_bool reconnect = 1;
    mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect);
#endif

    // 타임아웃(초)
    unsigned int connectTimeout = 5;
    unsigned int readTimeout = 5;
    unsigned int writeTimeout = 5;
    mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &connectTimeout);
    mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &readTimeout);
    mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &writeTimeout);

    if (!mysql_real_connect(connection,
        host_.c_str(), user_.c_str(), password_.c_str(),
        database_.c_str(), port_, nullptr, 0))
    {
        mysql_close(connection);
        return nullptr;
    }
    return connection;
}

DatabaseConnectionPool::ConnectionGuard DatabaseConnectionPool::Acquire()
{
    std::unique_lock<std::mutex> lock(poolMutex);
    poolCondition.wait(lock, [this]() { return !availableConnections.empty(); });

    MYSQL* connection = availableConnections.front();
    availableConnections.pop();
    return ConnectionGuard(this, connection);
}

void DatabaseConnectionPool::Return(MYSQL* connection)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    availableConnections.push(connection);
    poolCondition.notify_one();
}

DatabaseConnectionPool::ConnectionGuard::~ConnectionGuard()
{
    if (owner && connection) {
        owner->Return(connection);
        owner = nullptr;
        connection = nullptr;
    }
}
