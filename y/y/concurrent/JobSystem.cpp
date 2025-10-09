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

#include "JobSystem.h"
#include "concurrent.h"

#include <y/core/Chrono.h>
#include <y/utils/format.h>

namespace y {
namespace concurrent {


JobSystem::JobHandle::JobHandle(JobSystem* p) : _parent(p) {
}

bool JobSystem::JobHandle::is_finished() const {
    return _data && _data->finished;
}

void JobSystem::JobHandle::wait() const {
    y_debug_assert(_parent && _data);
    _parent->wait(*this);
}

JobSystem::JobSystem(usize thread_count) {
    for(usize i = 0; i != thread_count; ++i) {
        _threads.emplace_back([this, i] {
            concurrent::set_thread_name(fmt_c_str("Worker thread #{}", i));
            worker();
        });
    }
}

JobSystem::~JobSystem() {
    {
        const auto lock = std::unique_lock(_lock);
        _run = false;
        _condition.notify_all();
    }

    for(auto& thread : _threads) {
        thread.join();
    }

    {
        const auto lock = std::unique_lock(_lock);
        y_always_assert(_jobs.is_empty(), "Incomplete jobs remaining");
    }
}

usize JobSystem::concurrency() const {
    return _threads.size();
}

bool JobSystem::is_empty() const {
    const std::unique_lock lock(_lock);
    return !_total_jobs;
}

void JobSystem::cancel_pending_jobs() {
    const std::unique_lock lock(_lock);
    _jobs.make_empty();
    _waiting = 0;
    _total_jobs = 0;
}

JobSystem::JobHandle JobSystem::schedule(JobFunc&& func, core::Span<JobHandle> deps, std::source_location loc) {
    JobHandle handle(this);
    {
        handle._data = std::make_shared<JobData>();
        handle._data->func = std::move(func);
        handle._data->location = loc;
    }

    const auto lock = std::unique_lock(_lock);

    if(!deps.is_empty()) {
        u32 dep_count = 0;
        for(const JobHandle& h : deps) {
            JobData* data = h._data.get();
            y_debug_assert(data);

            const auto job_lock = std::unique_lock(data->lock);
            if(!data->finished) {
                ++dep_count;
                data->outgoing_deps.emplace_back(handle._data);
            }
        }

        if(dep_count) {
            handle._data->dependencies = dep_count;
            ++_total_jobs;
            ++_waiting;
            return handle;
        }
    }

    y_debug_assert(!handle._data->dependencies);
    ++_total_jobs;
    _jobs.emplace_back(handle._data);
    _condition.notify_one();

    return handle;
}

void JobSystem::wait(core::Span<JobHandle> jobs) {
    for(const JobHandle& job : jobs) {
        y_debug_assert(job._parent == this);

        while(!job.is_finished()) {
            auto lock = std::unique_lock(_lock);
            Y_TODO(we deadlock if we wait here, so we have to spin)
            process_one(lock);
        }
    }

    y_debug_assert(std::all_of(jobs.begin(), jobs.end(), [](const JobHandle& j) { return j.is_finished(); }));
}

void JobSystem::worker() {
    for(;;) {
        auto lock = std::unique_lock(_lock);
        _condition.wait(lock, [this] { return !_jobs.is_empty() || (!_run && !_waiting); });

        if(!process_one(lock)) {
            y_debug_assert(lock.owns_lock());
            if(!_run && !_waiting) {
                break;
            }
        } else {
            y_debug_assert(!lock.owns_lock());
        }
    }
}

bool JobSystem::process_one(std::unique_lock<std::mutex>& lock) {
    if(_jobs.is_empty()) {
        return false;
    }

    const std::shared_ptr<JobData> job = _jobs.pop_front();
    lock.unlock();

    y_debug_assert(job);
    y_debug_assert(!job->dependencies);

    job->func();

    {
        usize scheduled = 0;
        lock.lock();

        {
            const auto job_lock = std::unique_lock(job->lock);

            job->finished = true;
            for(usize i = 0; i != job->outgoing_deps.size(); ++i) {
                auto& out = job->outgoing_deps[i];
                if(out->dependencies.fetch_sub(1) == 1) {
                    ++scheduled;
                    _jobs.emplace_back(std::move(out));
                    --_waiting;
                }
            }
        }

        if(scheduled) {
            scheduled == 1 ? _condition.notify_one() : _condition.notify_all();
        }

        lock.unlock();
    }

    --_total_jobs;

    return true;
}


}
}

