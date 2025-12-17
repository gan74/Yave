/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef Y_CONCURRENT_JOBSYSTEM_H
#define Y_CONCURRENT_JOBSYSTEM_H

#include <y/core/Vector.h>
#include <y/core/RingQueue.h>

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


class JobSystem : NonMovable {
    using JobFunc = std::function<void(u32)>;

    struct JobData : NonMovable {
        JobFunc func;
        u32 count = 0;

        u32 started = 0;
        std::atomic<u32> finished = 0;

        std::atomic<u32> dependencies = 0;
        core::SmallVector<std::shared_ptr<JobData>> outgoing_deps;

        std::source_location location;
    };

    public:
        class JobHandle {
            public:
                JobHandle() = default;

                bool is_empty() const;
                bool is_finished() const;

                void wait() const;

                JobSystem* parent() const;

            private:
                friend class JobSystem;

                JobHandle(JobSystem* p);

                std::shared_ptr<JobData> _data;
                JobSystem* _parent = nullptr;
        };

        JobSystem(usize thread_count  = std::max(4u, std::thread::hardware_concurrency()));
        ~JobSystem();

        usize concurrency() const;

        bool is_empty() const;
        void cancel_pending_jobs();

        void wait(core::Span<JobHandle> jobs);

        template<typename F>
        JobHandle schedule(F&& func, core::Span<JobHandle> deps = {}, std::source_location loc = std::source_location::current()) {
            return schedule_n([func](u32) { func(); }, 1, deps, loc);
        }

        template<typename It, typename F>
        JobHandle parallel_for_async(It begin, It end, F&& func, core::Span<JobHandle> deps = {}, std::source_location loc = std::source_location::current()) {
            const usize size = usize(end - begin);
            const u32 target_task_count = u32(_threads.size() * 4);
            const u32 granularity = size ? u32(size / target_task_count) + 1 : 0;
            const u32 task_count = size ? u32((size / granularity) + (size % granularity ? 1 : 0)) : 1;
            return schedule_n([=](u32 index) {
                const It a = It(begin + usize(index * granularity));
                const It b = It(begin + std::min(size, usize((index + 1) * granularity)));
                func(a, b);
            }, task_count, deps, loc); // Need min 1 task to handle deps
        }


        template<typename It, typename F>
        void parallel_for(It begin, It end, F&& func, core::Span<JobHandle> deps = {}, std::source_location loc = std::source_location::current()) {
            if(begin != end || !deps.is_empty()) {
                parallel_for_async(begin, end, y_fwd(func), deps, loc).wait();
            }
        }

    private:
        JobHandle schedule_n(JobFunc&& func, u32 count, core::Span<JobHandle> deps, std::source_location loc);

        void worker();
        bool process_one(std::unique_lock<std::mutex>& lock);

        mutable std::mutex _lock;
        std::condition_variable _condition;

        core::RingQueue<std::shared_ptr<JobData>> _jobs;

        core::Vector<std::thread> _threads;
        std::atomic<u32> _waiting = 0;
        std::atomic<u32> _total_jobs = 0;

        bool _run = true;
};

}
}

#endif // Y_CONCURRENT_JOBSYSTEM_H

