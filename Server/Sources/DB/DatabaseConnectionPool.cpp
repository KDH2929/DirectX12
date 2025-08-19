#include "DatabaseConnectionPool.h"
#include <mutex>

static void EnsureMysqlLibraryInitializedOnce()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() {
        // ���� �� ���ܸ� ������ �ʰ�, Initialize �ʿ��� false�� ��ȯ�ϰ� �Ϸ���
        // ���¸� �ܺη� �����ؾ� ������, libmysql�� 0�̸� �����̶� ���⼭ �ܼ� ȣ��.
        mysql_library_init(0, nullptr, nullptr);
        });
}

DatabaseConnectionPool::~DatabaseConnectionPool()
{
    // ť�� ���� ���� ����
    while (!availableConnections.empty()) {
        MYSQL* connection = availableConnections.front();
        availableConnections.pop();
        if (connection) mysql_close(connection);
    }
    // mysql_library_end()�� ���μ��� ���� ������ ȣ���ص� �ǰ� �����ص� ��(���� ��� �� ���� ���������� ����)
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

    // �̸� N�� ����
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

    // �ڵ� �翬��
#if defined(MYSQL_VERSION_ID) && (MYSQL_VERSION_ID >= 80000)
    bool reconnect = true;
    mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect);
#else
    my_bool reconnect = 1;
    mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect);
#endif

    // Ÿ�Ӿƿ�(��)
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
