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
    public:
        using JobFunc = std::function<void()>;

    private:
        struct JobData : NonMovable {
            JobFunc func;
            std::atomic<u32> dependencies = 0;

            std::mutex lock;
            core::SmallVector<std::shared_ptr<JobData>> outgoing_deps;
            bool finished = false;
        };

    public:
        class JobHandle {
            public:
            private:
                friend class JobSystem;

                std::shared_ptr<JobData> _data;
        };


        /*class JobSet {
            public:
                void add_job(JobFunc&& func) {
                    _jobs.emplace_back(std::move(func));
                }

            private:
                core::Vector<JobFunc> _jobs;
        };*/


        JobSystem(usize thread_count  = std::max(4u, std::thread::hardware_concurrency()));
        ~JobSystem();


        JobHandle schedule(JobFunc&& func, core::Span<JobHandle> deps = {});


    private:
        void process_one(std::unique_lock<std::mutex>& lock);
        void worker();

        mutable std::mutex _lock;
        std::condition_variable _condition;

        core::RingQueue<std::shared_ptr<JobData>> _jobs;

        core::Vector<std::thread> _threads;
        std::atomic<u32> _waiting = 0;

        bool _run = true;
};

}
}

#endif // Y_CONCURRENT_JOBSYSTEM_H

