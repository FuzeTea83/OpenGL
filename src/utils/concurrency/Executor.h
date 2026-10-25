#ifndef OPENGL_EXECUTOR_H
#define OPENGL_EXECUTOR_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class Executor
{
public:
    using Runnable = std::function<void()>;

    explicit Executor(unsigned int numThreads);

    Executor(const Executor &) = delete;

    Executor &operator=(const Executor &) = delete;

    Executor(Executor &&) = delete;

    Executor &operator=(Executor &&) = delete;

    ~Executor() { shutdown(); }

    void submit(Runnable func);

    template<typename F, typename... Args>
    auto submitWithResult(F &&f, Args &&... args) -> std::future<std::invoke_result_t<F, Args...> >
    {
        using Result = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<Result()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<Result> fut = task->get_future();
        submit([task] { (*task)(); });
        return fut;
    }

    void shutdown();

    std::vector<std::thread::id> getWorkerThreadIds() const;

    bool isSingleThreaded() const { return mWorkers.size() == 1; }

private:
    void workerLoop();

    unsigned int mNumThreads = 0;
    std::vector<std::thread> mWorkers;
    std::mutex mMutex;
    std::condition_variable mCv;
    std::queue<Runnable> mTasks;
    std::atomic<bool> mStopping{false};
};

#endif //OPENGL_EXECUTOR_H
