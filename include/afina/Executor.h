#ifndef AFINA_THREADPOOL_H
#define AFINA_THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace Afina {

/**
 * # Thread pool
 */
class Executor {
    enum class State {
        // Threadpool is fully operational, tasks could be added and get executed
        kRun,

        // Threadpool is on the way to be shutdown, no ned task could be added, but existing will be
        // completed as requested
        kStopping,

        // Threadppol is stopped
        kStopped
    };

    Executor(std::string name, int size);
    ~Executor(void);

    /**
     * Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
     * free. All enqueued jobs will be complete.
     *
     * In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
     */
    void Stop(bool await = false);

    /**
     * Add function to be executed on the threadpool. Method returns true in case if task has been placed
     * onto execution queue, i.e scheduled for execution and false otherwise.
     *
     * That function doesn't wait for function result. Function could always be written in a way to notify caller about
     * execution finished by itself
     */
    template <typename F, typename... Types> bool Execute(F &&func, Types... args) {
        // Prepare "task"
        auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);

        std::unique_lock<std::mutex> lock(this->mutex);
        if (state != State::kRun) {
            return false;
        }

        // Enqueue new task
        tasks.push_back(exec);
        empty_condition.notify_one();
        return true;
    }

private:
    // No copy/move/assign allowed
    Executor(const Executor &)             = delete;
    Executor(Executor &&)                  = delete;
    Executor &operator=(const Executor &)  = delete;
    Executor &operator=(Executor &&)       = delete;

    /**
     * Main function that all pool threads are running. It polls internal task queue and execute tasks
     */
    friend void perform(Executor *executor);

    /**
     * Mutex to protect state below from concurrent modification
     */
    std::mutex mutex;

    /**
     * Conditional variable to await new data in case of empty queue
     */
    std::condition_variable empty_condition;

    /**
     * Vector of actual threads that perorm execution
     */
    std::vector<std::thread> threads;

    /**
     * Task queue
     */
    std::deque<std::function<void()>> tasks;

    /**
     * Flag to stop bg threads
     */
    State state;
};

Executor::Executor(std::string name, int size) : threads(size, [&](void) { perform(this); }) {
    state = State::kRun;
}

Executor::~Executor(void) {
    this->Stop(true);
}

void Executor::Stop(bool await) {
    mutex.lock();
    if (state == State::kRun) {
        state = State::kStopping;
        empty_condition.notify_all();
        mutex.unlock();
        if (await) {
            std::for_each(threads.begin(), threads.end(), [](std::thread &t) { t.join(); });
        }
        mutex.lock();
        state = State::kStopped;
    }
    mutex.unlock();
}

void perform(Executor *executor) {
    for (;;) {
        std::function<void(void)> task;
        {
            std::unique_lock<std::mutex> lock(executor->mutex);
            executor->empty_condition.wait(lock, [&executor](void) {
                return executor->state == Executor::State::kStopping || !executor->tasks.empty();
            });
            if (executor->state == Executor::State::kStopping && executor->tasks.empty()) {
                break;
            }
            task = executor->tasks.front();
            executor->tasks.pop_front();
        }
        task();
    }
}

} // namespace Afina

#endif // AFINA_THREADPOOL_H
