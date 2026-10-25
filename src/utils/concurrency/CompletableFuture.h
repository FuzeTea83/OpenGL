#ifndef OPENGL_COMPLETABLE_FUTURE_H
#define OPENGL_COMPLETABLE_FUTURE_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "Executor.h"

template<typename T>
class CompletableFuture;

namespace detail
{
template<typename T>
struct FutureState
{
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    std::exception_ptr exception;
    std::vector<std::function<void()> > callbacks;

    void complete(T value)
    {
        std::vector<std::function<void()> > toRun;
        {
            std::lock_guard lock(mtx);
            if (done) { return; }
            result = std::move(value);
            done = true;
            toRun.swap(callbacks);
        }
        cv.notify_all();
        for (auto &cb: toRun) { cb(); }
    }

    void completeExceptionally(const std::exception_ptr &ex)
    {
        std::vector<std::function<void()> > toRun;
        {
            std::lock_guard lock(mtx);
            if (done) { return; }
            exception = ex;
            done = true;
            toRun.swap(callbacks);
        }
        cv.notify_all();
        for (auto &cb: toRun) { cb(); }
    }

    template<typename F>
    void addCallback(F &&f)
    {
        bool runNow = false;
        {
            std::lock_guard lock(mtx);
            if (!done)
            {
                callbacks.push_back(std::forward<F>(f));
                return;
            }
            runNow = true;
        }
        if (runNow) { f(); }
    }

    std::variant<T, std::exception_ptr> result;
};

template<>
struct FutureState<void>
{
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    std::exception_ptr exception;
    std::vector<std::function<void()> > callbacks;

    void complete()
    {
        std::vector<std::function<void()> > toRun;
        {
            std::lock_guard lock(mtx);
            if (done) { return; }
            done = true;
            toRun.swap(callbacks);
        }
        cv.notify_all();
        for (auto &cb: toRun) { cb(); }
    }

    void completeExceptionally(const std::exception_ptr &ex)
    {
        std::vector<std::function<void()> > toRun;
        {
            std::lock_guard lock(mtx);
            if (done) { return; }
            exception = ex;
            done = true;
            toRun.swap(callbacks);
        }
        cv.notify_all();
        for (auto &cb: toRun) { cb(); }
    }

    template<typename F>
    void addCallback(F &&f)
    {
        bool runNow = false;
        {
            std::lock_guard lock(mtx);
            if (!done)
            {
                callbacks.push_back(std::forward<F>(f));
                return;
            }
            runNow = true;
        }
        if (runNow) { f(); }
    }
};
} // namespace detail


template<>
class CompletableFuture<void>
{
    std::shared_ptr<detail::FutureState<void> > mState;

public:
    using State = detail::FutureState<void>;

    template<typename F>
    static CompletableFuture runAsync(F &&func, Executor &executor)
    {
        auto cf = CompletableFuture();
        executor.submit([cf, f = std::forward<F>(func)]() mutable
        {
            try
            {
                f();
                cf.complete();
            }
            catch (...) { cf.completeExceptionally(std::current_exception()); }
        });
        return cf;
    }

    static CompletableFuture completed()
    {
        auto cf = CompletableFuture();
        cf.complete();
        return cf;
    }

    static CompletableFuture failed(const std::exception_ptr &ex)
    {
        auto cf = CompletableFuture();
        cf.completeExceptionally(ex);
        return cf;
    }

    template<typename F>
    CompletableFuture thenAccept(F &&f, Executor &executor)
    {
        auto next = CompletableFuture();
        scheduleCallback(executor, [next, f = std::forward<F>(f), mState = this->mState]() mutable
        {
            if (mState->exception)
            {
                next.completeExceptionally(mState->exception);
                return;
            }
            try
            {
                f();
                next.complete();
            }
            catch (...) { next.completeExceptionally(std::current_exception()); }
        });
        return next;
    }

    template<typename F>
    CompletableFuture exceptionally(F &&f, Executor &executor)
    {
        auto next = CompletableFuture();
        scheduleCallback(executor, [next, f = std::forward<F>(f), mState = this->mState]() mutable
        {
            if (!mState->exception)
            {
                next.complete();
                return;
            }
            try
            {
                f(mState->exception);
                next.complete();
            }
            catch (...) { next.completeExceptionally(std::current_exception()); }
        });
        return next;
    }

    void get() const
    {
        std::unique_lock lock(mState->mtx);
        mState->cv.wait(lock, [mState = this->mState] { return mState->done; });
        if (mState->exception) { std::rethrow_exception(mState->exception); }
    }

    bool isDone() const
    {
        std::lock_guard lock(mState->mtx);
        return mState->done;
    }

    bool isCompletedExceptionally() const
    {
        std::lock_guard lock(mState->mtx);
        return mState->done && mState->exception != nullptr;
    }

    static CompletableFuture whenAll(std::vector<CompletableFuture> futures, Executor &executor)
    {
        auto cf = CompletableFuture();
        if (futures.empty())
        {
            cf.complete();
            return cf;
        }

        auto remaining = std::make_shared<std::atomic<int> >(static_cast<int>(futures.size()));
        auto hasError = std::make_shared<std::atomic<bool> >(false);
        auto firstEx = std::make_shared<std::exception_ptr>(nullptr);
        auto exMtx = std::make_shared<std::mutex>();

        auto onDone = [remaining, hasError, firstEx, exMtx, cf]
        {
            if (--*remaining == 0)
            {
                if (*hasError)
                {
                    std::lock_guard l(*exMtx);
                    cf.completeExceptionally(*firstEx);
                }
                else { cf.complete(); }
            }
        };

        for (auto &f: futures)
        {
            f.mState->addCallback([onDone, f, hasError, firstEx, exMtx]
            {
                if (f.mState->exception)
                {
                    std::lock_guard l(*exMtx);
                    if (!*hasError)
                    {
                        *hasError = true;
                        *firstEx = f.mState->exception;
                    }
                }
                onDone();
            });
        }
        return cf;
    }

private:
    CompletableFuture() : mState(std::make_shared<State>()) {}

    void complete() const { mState->complete(); }
    void completeExceptionally(const std::exception_ptr &ex) const { mState->completeExceptionally(ex); }

    template<typename Callback>
    void scheduleCallback(Executor &executor, Callback &&cb)
    {
        mState->addCallback([&executor, cb = std::forward<Callback>(cb)]() mutable { executor.submit(std::move(cb)); });
    }

    template<typename U>
    friend class CompletableFuture;
};


template<typename T>
class CompletableFuture
{
    std::shared_ptr<detail::FutureState<T> > mState;

public:
    using value_type = T;
    using State = detail::FutureState<T>;

    template<typename F>
    static CompletableFuture runAsync(F &&func, Executor &executor)
    {
        auto cf = CompletableFuture();
        executor.submit([cf, f = std::forward<F>(func)]() mutable
        {
            try { cf.complete(f()); }
            catch (...) { cf.completeExceptionally(std::current_exception()); }
        });
        return cf;
    }

    static CompletableFuture completed(T value)
    {
        auto cf = CompletableFuture();
        cf.complete(std::move(value));
        return cf;
    }

    static CompletableFuture failed(std::exception_ptr ex)
    {
        auto cf = CompletableFuture();
        cf.completeExceptionally(ex);
        return cf;
    }

    template<typename F>
    auto thenApply(F &&f, Executor &executor) -> CompletableFuture<std::decay_t<std::invoke_result_t<F, T> > >
    {
        using R = std::decay_t<std::invoke_result_t<F, T> >;
        auto next = CompletableFuture<R>{};
        scheduleCallback(executor, [next, f = std::forward<F>(f), mState = this->mState]() mutable
        {
            if (mState->exception)
            {
                next.completeExceptionally(mState->exception);
                return;
            }
            try { next.complete(f(std::get<T>(mState->result))); }
            catch (...) { next.completeExceptionally(std::current_exception()); }
        });
        return next;
    }

    template<typename F>
    auto thenCompose(
        F &&f, Executor &executor) -> CompletableFuture<typename std::decay_t<std::invoke_result_t<F, T> >::value_type>
    {
        using Inner = std::decay_t<std::invoke_result_t<F, T> >;
        using U = Inner::value_type;
        auto next = CompletableFuture<U>{};
        scheduleCallback(executor, [next, f = std::forward<F>(f), &executor, mState = this->mState]() mutable
        {
            if (mState->exception)
            {
                next.completeExceptionally(mState->exception);
                return;
            }
            try
            {
                Inner inner = f(std::get<T>(mState->result));
                inner.mState->addCallback([inner, next, &executor]()
                {
                    executor.submit([inner, next]()
                    {
                        if (inner.mState->exception) { next.completeExceptionally(inner.mState->exception); }
                        else { next.complete(std::get<U>(inner.mState->result)); }
                    });
                });
            }
            catch (...) { next.completeExceptionally(std::current_exception()); }
        });
        return next;
    }

    template<typename F>
    CompletableFuture<void> thenAccept(F &&f, Executor &executor)
    {
        auto next = CompletableFuture<void>{};
        scheduleCallback(executor, [next, f = std::forward<F>(f), mState = this->mState]() mutable
        {
            if (mState->exception)
            {
                next.completeExceptionally(mState->exception);
                return;
            }
            try
            {
                f(std::get<T>(mState->result));
                next.complete();
            }
            catch (...) { next.completeExceptionally(std::current_exception()); }
        });
        return next;
    }

    template<typename F>
    CompletableFuture exceptionally(F &&f, Executor &executor)
    {
        auto next = CompletableFuture{};
        scheduleCallback(executor, [next, f = std::forward<F>(f), mState = this->mState]() mutable
        {
            if (!mState->exception)
            {
                next.complete(std::get<T>(mState->result));
                return;
            }
            try { next.complete(f(mState->exception)); }
            catch (...) { next.completeExceptionally(std::current_exception()); }
        });
        return next;
    }

    T get()
    {
        std::unique_lock<std::mutex> lock(mState->mtx);
        mState->cv.wait(lock, [mState = this->mState] { return mState->done; });
        if (std::holds_alternative<std::exception_ptr>(mState->result))
            std::rethrow_exception(
                std::get<std::exception_ptr>(mState->result));
        return std::get<T>(mState->result);
    }

    bool isDone() const
    {
        std::lock_guard<std::mutex> lock(mState->mtx);
        return mState->done;
    }

    bool isCompletedExceptionally() const
    {
        std::lock_guard<std::mutex> lock(mState->mtx);
        return mState->done && std::holds_alternative<std::exception_ptr>(mState->result);
    }

    template<typename U>
    static CompletableFuture<std::vector<U> > whenAll(std::vector<CompletableFuture<U> > futures, Executor &executor)
    {
        auto cf = CompletableFuture<std::vector<U> >{};
        if (futures.empty())
        {
            cf.complete({});
            return cf;
        }

        auto remaining = std::make_shared<std::atomic<int> >(static_cast<int>(futures.size()));
        auto results = std::make_shared<std::vector<U> >(futures.size());
        auto hasError = std::make_shared<std::atomic<bool> >(false);
        auto firstEx = std::make_shared<std::exception_ptr>(nullptr);
        auto exMtx = std::make_shared<std::mutex>();

        auto onDone = [remaining, results, hasError, firstEx, exMtx, cf]()
        {
            if (--(*remaining) == 0)
            {
                if (*hasError)
                {
                    std::lock_guard l(*exMtx);
                    cf.completeExceptionally(*firstEx);
                }
                else { cf.complete(std::move(*results)); }
            }
        };

        for (std::size_t i = 0; i < futures.size(); ++i)
        {
            futures[i].mState->addCallback([i, onDone, futures, results, hasError, firstEx, exMtx]
            {
                if (futures[i].mState->exception)
                {
                    std::lock_guard l(*exMtx);
                    if (!*hasError)
                    {
                        *hasError = true;
                        *firstEx = futures[i].mState->exception;
                    }
                }
                else { (*results)[i] = std::get<U>(futures[i].mState->result); }
                onDone();
            });
        }
        return cf;
    }

private:
    CompletableFuture() : mState(std::make_shared<State>()) {}

    void complete(T value) const { mState->complete(std::move(value)); }
    void completeExceptionally(std::exception_ptr ex) const { mState->completeExceptionally(ex); }

    template<typename Callback>
    void scheduleCallback(Executor &executor, Callback &&cb)
    {
        mState->addCallback([&executor, cb = std::forward<Callback>(cb)]() mutable { executor.submit(std::move(cb)); });
    }

    friend class CompletableFuture<void>;
};

#endif //OPENGL_COMPLETABLE_FUTURE_H
