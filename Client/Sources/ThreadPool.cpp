#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads_) 
    : numThreads(numThreads_)
{
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this]() { WorkerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    // Pool ���� �÷��� ���� �� ��� worker �����
    shutdownFlag = true;
    taskAvailableCondition.notify_all();

    // �� �����尡 ������ ����� ������ ��ٸ�
    for (auto& worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::Submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(taskQueueMutex);
        taskQueue.push(std::move(task));   // move�� ���� ��� ���� (���� ������ ��� �̵������� ȣ��)
        ++pendingTaskCount;                // ��� ���� �۾� �� ����
    }
    taskAvailableCondition.notify_one();   // �۾� ���� signal
}

void ThreadPool::Wait() {
    std::unique_lock<std::mutex> lock(completionMutex);
    // ��� �۾�(pendingTaskCount == 0) �� ���� ������ ���
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
            // �۾��� ���ų�, ���� �÷��װ� ������ ������ ���
            taskAvailableCondition.wait(lock, [this]() {
                return shutdownFlag || !taskQueue.empty();
                });

            // ���� �÷��� && ���� �۾��� ������ ���� ����
            if (shutdownFlag && taskQueue.empty())
                return;

            // Task ��������
            task = std::move(taskQueue.front());
            taskQueue.pop();
        }

        // ���� Task ����
        task();

        // ����������� �۾� �� ����
        // ������ 0�� �Ǹ� Wait() ���� ���ν����� �����
        if (--pendingTaskCount == 0) {
            std::lock_guard<std::mutex> lock(completionMutex);
            allTasksDoneCondition.notify_one();
        }
    }
}
