#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    // 작업을 스레드풀에 제출
    void Submit(std::function<void()> task);
    // 모든 작업이 끝날 때까지 호출 스레드를 대기
    void Wait();

    size_t GetThreadCount() const;

private:
    void WorkerLoop();


private:

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> taskQueue;

    std::mutex taskQueueMutex;
    std::condition_variable taskAvailableCondition;

    std::atomic<bool> shutdownFlag{ false };
    std::atomic<int>  pendingTaskCount{ 0 };

    std::mutex completionMutex;
    std::condition_variable allTasksDoneCondition;

    size_t numThreads;
};