#include "Executor.h"

#include <iostream>

Executor::Executor(const unsigned int numThreads)
{
    if (numThreads == 0) { throw std::invalid_argument("Number of threads must be greater than 0"); }
    mWorkers.reserve(numThreads);
    for (unsigned int i = 0; i < numThreads; ++i) { mWorkers.emplace_back([this] { workerLoop(); }); }
}

void Executor::submit(Runnable func)
{
    {
        std::lock_guard lock(mMutex);
        if (mStopping) { return; }
        mTasks.push(std::move(func));
    }
    mCv.notify_one();
}

void Executor::shutdown()
{
    {
        std::lock_guard lock(mMutex);
        if (mStopping.exchange(true)) { return; }
    }
    mCv.notify_all();
    for (std::thread &t: mWorkers) { if (t.joinable()) t.join(); }
}

std::vector<std::thread::id> Executor::getWorkerThreadIds() const
{
    std::vector<std::thread::id> ids;
    ids.reserve(mWorkers.size());
    for (const auto &worker: mWorkers) { ids.push_back(worker.get_id()); }
    return ids;
}

void Executor::workerLoop()
{
    while (true)
    {
        Runnable task;
        {
            std::unique_lock lock(mMutex);
            mCv.wait(lock, [this] { return mStopping || !mTasks.empty(); });
            if (mTasks.empty()) { return; } // mStopping == true
            task = std::move(mTasks.front());
            mTasks.pop();
        }
        try { task(); }
        catch (...)
        {
            std::cerr << "Exception caught in Executor worker loop" << std::endl;
            try { throw; }
            catch (const std::exception &e) { std::cerr << "Caught std::exception: " << e.what() << std::endl; }
            catch (const char *msg) { std::cerr << "Caught C-string: " << msg << std::endl; }
            catch (...) { std::cerr << "Unknown exception type." << std::endl; }
        }
    }
}
