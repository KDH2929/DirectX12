#pragma once

#include <functional>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <cstdint>


struct MYSQL;
class DatabaseConnectionPool;

class DbExecutor {
public:
    using Task = std::function<void(MYSQL*)>; // ���� Ŀ�ؼ��� �޾� �۾� ����

    DbExecutor() = default;
    ~DbExecutor(); // Stop() ȣ��

    DbExecutor(const DbExecutor&) = delete;
    DbExecutor& operator=(const DbExecutor&) = delete;


    void Start(int threadCount, std::shared_ptr<DatabaseConnectionPool> pool);
    void Stop();                   
    bool Enqueue(Task task);       

private:
    void WorkerLoop();

private:
    std::vector<std::thread> workers;
    std::deque<Task> taskQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::atomic<bool> running{ false };
    std::shared_ptr<DatabaseConnectionPool> connectionPool;
};