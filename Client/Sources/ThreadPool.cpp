#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads_) 
    : numThreads(numThreads_)
{
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this]() { WorkerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    // Pool 종료 플래그 설정 및 모든 worker 깨우기
    shutdownFlag = true;
    taskAvailableCondition.notify_all();

    // 각 스레드가 안전히 종료될 때까지 기다림
    for (auto& worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::Submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(taskQueueMutex);
        taskQueue.push(std::move(task));   // move로 복사 비용 절감 (복사 생성자 대신 이동생성자 호출)
        ++pendingTaskCount;                // 대기 중인 작업 수 증가
    }
    taskAvailableCondition.notify_one();   // 작업 가능 signal
}

void ThreadPool::Wait() {
    std::unique_lock<std::mutex> lock(completionMutex);
    // 모든 작업(pendingTaskCount == 0) 이 끝날 때까지 대기
    allTasksDoneCondition.wait(lock, [this]() {
        return pendingTaskCount.load() == 0;
        });
}

size_t ThreadPool::GetThreadCount() const
{
    return numThreads;
}

void ThreadPool::WorkerLoop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(taskQueueMutex);
            // 작업이 없거나, 종료 플래그가 설정될 때까지 대기
            taskAvailableCondition.wait(lock, [this]() {
                return shutdownFlag || !taskQueue.empty();
                });

            // 종료 플래그 && 남은 작업이 없으면 루프 종료
            if (shutdownFlag && taskQueue.empty())
                return;

            // Task 꺼내오기
            task = std::move(taskQueue.front());
            taskQueue.pop();
        }

        // 실제 Task 수행
        task();

        // 현재수행중인 작업 수 감소
        // 개수가 0이 되면 Wait() 중인 메인스레드 깨우기
        if (--pendingTaskCount == 0) {
            std::lock_guard<std::mutex> lock(completionMutex);
            allTasksDoneCondition.notify_one();
        }
    }
}
