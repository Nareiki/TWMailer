#include "helpers/threadPool.h"

/**
 * @brief Constructs a thread pool with a fixed number of worker threads.
 *        Each thread waits on a shared task queue and processes tasks as they arrive.
 * @param numThreads Number of threads to spawn.
 */
ThreadPool::ThreadPool(size_t numThreads) : stop_(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this]() {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    condition_.wait(lock, [this]() {
                        return stop_ || !tasks_.empty();
                    });

                    if (stop_ && tasks_.empty()) {
                        return;
                    }

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                task();
            }
        });
    }
}

/**
 * @brief Adds a new task to the thread pool's task queue.
 *        Notifies one waiting thread to pick up the task.
 * @param task The task to enqueue.
 * @throws std::runtime_error if the thread pool has already been stopped.
 */
void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        if (stop_) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks_.emplace(std::move(task));
    }
    condition_.notify_one();
}

/**
 * @brief Destructor: Signals all threads to stop and joins them.
 *        Ensures no tasks are left unprocessed and all threads exit cleanly.
 */
ThreadPool::~ThreadPool() {
    stop_ = true;
    condition_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}
