/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef Y_CONCURRENT_STATICTHREADPOOL_H
#define Y_CONCURRENT_STATICTHREADPOOL_H

#include <y/core/Vector.h>

#include "concurrent.h"

#include <list>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <condition_variable>

namespace y {
namespace concurrent {

class StaticThreadPool;

class DependencyGroup {
    public:
        DependencyGroup() = default;

        bool is_empty() const;
        bool is_ready() const;
        bool is_expired() const;
        u32 dependency_count() const;

    private:
        friend class StaticThreadPool;

        void add_dependency();
        void solve_dependency();

        std::shared_ptr<std::atomic<u32>> _counter;
};

class StaticThreadPool : NonMovable {
    private:
        using Func = std::function<void()>;

        struct FuncData {
            FuncData(Func func, DependencyGroup wait, DependencyGroup done = DependencyGroup());

            Func function;
            DependencyGroup wait_for;
            DependencyGroup on_done;
        };

        struct SharedData {
            mutable std::mutex lock;
            std::condition_variable condition;

            std::list<FuncData> queue;

            std::atomic<bool> run = true;
        };

    public:

        // Thread names must have static storage
        StaticThreadPool(usize thread_count = std::max(4u, std::thread::hardware_concurrency()), const char* thread_names = nullptr);
        ~StaticThreadPool();

        usize concurency() const;
        usize pending_tasks() const;

        // Empty means all tasks are scheduled, not done!
        void process_until_empty();

        void schedule(Func&& func, DependencyGroup* on_done = nullptr, DependencyGroup wait_for = DependencyGroup());

        template<typename F, typename R = decltype(std::declval<F>()())>
        std::future<R> schedule_with_future(F&& func, DependencyGroup* on_done = nullptr, DependencyGroup wait_for = DependencyGroup()) {
            auto promise = std::make_shared<std::promise<R>>();
            auto future = promise->get_future();
            schedule(std::move([p = std::move(promise), f = y_fwd(func)]() { p->set_value(f()); }), on_done, wait_for);
            return future;
        }

    private:
        bool process_one(std::unique_lock<std::mutex> lock);
        void worker();

        SharedData _shared_data;
        core::Vector<std::thread> _threads;
};

class WorkerThread : public StaticThreadPool {
    public:
        WorkerThread(const char* thread_name = nullptr) : StaticThreadPool(1, thread_name) {
        }
};

}
}

#endif // Y_CONCURRENT_STATICTHREADPOOL_H

