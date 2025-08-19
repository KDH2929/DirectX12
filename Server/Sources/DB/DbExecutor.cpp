#include "DbExecutor.h"
#include "DatabaseConnectionPool.h" 

DbExecutor::~DbExecutor() {
    Stop();
}

void DbExecutor::Start(int threadCount, std::shared_ptr<DatabaseConnectionPool> pool) {
    if (running.load(std::memory_order_acquire)) return;
    if (!pool) return;

    if (threadCount <= 0) {
        const unsigned hardwareConcurrency = std::thread::hardware_concurrency();
        threadCount = (hardwareConcurrency > 0) ? static_cast<int>(hardwareConcurrency) : 2;
    }

    connectionPool = std::move(pool);
    workers.reserve(threadCount);
    running.store(true, std::memory_order_release);

    for (int i = 0; i < threadCount; ++i) {
        workers.emplace_back([this]() { WorkerLoop(); });
    }
}

void DbExecutor::Stop() {
    if (!running.exchange(false)) return;


    // 만약 남은 작업 끝날 때까지 기다리지 않고 바로 끝내야한다면 주석해제
    /*
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.clear();
    }
    */

    queueCondition.notify_all();

    for (auto& task : workers) {
        if (task.joinable()) task.join();
    }
    workers.clear();
    taskQueue.clear();
    connectionPool.reset();
}

bool DbExecutor::Enqueue(Task task) {
    if (!running.load(std::memory_order_acquire)) return false;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.emplace_back(std::move(task));
    }
    queueCondition.notify_one();
    return true;
}

void DbExecutor::WorkerLoop() {
    while (true) {
        Task task;

        std::unique_lock<std::mutex> lock(queueMutex);
        queueCondition.wait(lock, [this] {
            return !running.load(std::memory_order_acquire) || !taskQueue.empty();
            });

        if (!running.load(std::memory_order_relaxed) && taskQueue.empty()) {
            lock.unlock();           // 명시적으로 해제
            break;                   //  break 로 반복문 빠져나오기
        }

        task = std::move(taskQueue.front());
        taskQueue.pop_front();
        lock.unlock();               // 작업 실행 전 명시적 해제

        try {
            auto guard = connectionPool->Acquire();
            MYSQL* connection = guard.Get();
            task(connection);
        }
        catch (const std::exception&) {
            // TODO: log
        }
        catch (...) {
            // TODO: log
        }
    }
}