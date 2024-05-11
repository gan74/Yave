/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <condition_variable>
#include <source_location>
#include <optional>
#include <latch>


namespace y {
namespace concurrent {

class StaticThreadPool;

class DependencyGroup {
    struct Data {
        std::atomic<u32> counter = 0;
        u32 max = 0;

        bool is_ready() const;
        bool notify();
    };

    public:
        DependencyGroup();

        void reset();
        void init();

        bool is_empty() const;
        bool is_ready() const;

    private:
        friend class StaticThreadPool;

        std::shared_ptr<DependencyGroup::Data> create_signal();

        std::shared_ptr<Data> _data;
};

class StaticThreadPool : NonMovable {
    private:
        using Func = std::function<void()>;

        struct Task : NonCopyable {
            Task(Func func, core::Span<DependencyGroup> wait, std::shared_ptr<DependencyGroup::Data> sig, std::source_location loc);

            bool is_ready() const;

            Func function;
            core::SmallVector<DependencyGroup, 4> wait_for;
            std::shared_ptr<DependencyGroup::Data> signal;

#ifdef Y_DEBUG
            std::source_location location;
#endif
        };

    public:
        StaticThreadPool(usize thread_count = std::max(4u, std::thread::hardware_concurrency()));
        ~StaticThreadPool();

        usize concurency() const;
        bool is_empty() const;
        usize pending_tasks() const;

        void cancel_pending_tasks();

        void process_until_complete(core::Span<DependencyGroup> wait_for);

        void schedule(Func&& func, DependencyGroup* signal = nullptr, core::Span<DependencyGroup> wait_for = {}, std::source_location loc = std::source_location::current());

        template<typename F, typename R = decltype(std::declval<F>()())>
        std::future<R> schedule_with_future(F&& func, DependencyGroup* on_done = nullptr, core::Span<DependencyGroup> wait_for = {}) {
            auto promise = std::make_shared<std::promise<R>>();
            auto future = promise->get_future();
            schedule(std::move([p = std::move(promise), f = y_fwd(func)]() { p->set_value(f()); }), on_done, wait_for);
            return future;
        }


    private:
        bool process_one(std::unique_lock<std::mutex> lock);
        void worker();

        mutable std::mutex _lock;

        std::condition_variable _condition;
        std::atomic<u64> _generation = 0;

        Y_TODO(change to ring queue)
        core::Vector<std::shared_ptr<Task>> _queue;

        std::atomic<u32> _working_threads = 0;
        std::atomic<bool> _run = true;
        core::Vector<std::thread> _threads;
};

class WorkerThread : public StaticThreadPool {
    public:
        WorkerThread() : StaticThreadPool(1) {
        }
};

}
}

#endif // Y_CONCURRENT_STATICTHREADPOOL_H

