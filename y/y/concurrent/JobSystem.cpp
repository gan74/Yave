/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

bool JobSystem::JobHandle::is_empty() const {
    return !_data;
}

bool JobSystem::JobHandle::is_finished() const {
    return _data && _data->finished == _data->count;
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

/*void JobSystem::cancel_pending_jobs() {
    // This is incorrect as it doesn't account for dependencies
    const std::unique_lock lock(_lock);
    _jobs.make_empty();
    _waiting = 0;
    _total_jobs = 0;
}*/

JobSystem::JobHandle JobSystem::schedule_n(JobFunc&& func, u32 count, core::Span<JobHandle> deps, std::source_location loc) {
    y_debug_assert(count > 0);

    JobHandle handle(this);
    {
        handle._data = std::make_shared<JobData>();
        handle._data->func = std::move(func);
        handle._data->count = count;

        handle._data->location = loc;
    }

    const auto lock = std::unique_lock(_lock);

    if(!deps.is_empty()) {
        u32 dep_count = 0;
        for(const JobHandle& h : deps) {
            JobData* data = h._data.get();
            y_debug_assert(data);

            if(data->finished != data->count) {
                ++dep_count;
                data->outgoing_deps.emplace_back(handle._data);
            }
        }

        if(dep_count) {
            handle._data->dependencies = dep_count;
            _total_jobs += count;
            ++_waiting;
            return handle;
        }
    }

    y_debug_assert(!handle._data->dependencies);
    _total_jobs += count;
    _jobs.emplace_back(handle._data);
    ++_queued;

    wake(count);

    return handle;
}

void JobSystem::wait(core::Span<JobHandle> jobs) {
    for(const JobHandle& job : jobs) {
        y_debug_assert(job._parent == this);

        for(;;) {
            const u32 finished = job._data->finished;
            if(finished == job._data->count) {
                break;
            }

            auto lock = std::unique_lock(_lock);
            if(!process_one(lock, false)) {
                lock.unlock();
                job._data->finished.wait(finished);
            }
        }
    }

    y_debug_assert(std::all_of(jobs.begin(), jobs.end(), [](const JobHandle& j) { return j.is_finished(); }));
}

void JobSystem::wake(u32 count) {
    for(u32 i = 0; i < count && i < _sleeping; ++i) {
        _condition.notify_one();
    }
}

void JobSystem::worker() {
    for(;;) {
        /*const core::StopWatch spin_timer;
        while(_total_jobs && !_queued && spin_timer.elapsed() < core::Duration::microseconds(50)) {
        }*/

        auto lock = std::unique_lock(_lock);
        ++_sleeping;
        _condition.wait(lock, [this] { return !_jobs.is_empty() || (!_run && !_waiting); });
        --_sleeping;

        if(!process_one(lock, true)) {
            y_debug_assert(lock.owns_lock());
            if(!_run && !_waiting) {
                break;
            }
        } else {
            y_debug_assert(!lock.owns_lock());
        }
    }
}

bool JobSystem::process_one(std::unique_lock<std::mutex>& lock, bool run_next) {
    if(_jobs.is_empty()) {
        return false;
    }

    std::shared_ptr<JobData> job = _jobs.first();
    u32 index = job->started++;
    if(index + 1 == job->count) {
        _jobs.pop_front();
        --_queued;
    }

    lock.unlock();

    while(job) {
        y_debug_assert(!job->dependencies);

        job->func(index);

        std::shared_ptr<JobData> next;

        {
            u32 scheduled = 0;
            lock.lock();

            if(++job->finished == job->count) {
                job->finished.notify_all();
                for(usize i = 0; i != job->outgoing_deps.size(); ++i) {
                    auto& out = job->outgoing_deps[i];
                    if(out->dependencies.fetch_sub(1) == 1) {
                        --_waiting;
                        if(run_next && !next && out->count == 1) {
                            next = std::move(out);
                            next->started = 1;
                        } else {
                            scheduled += out->count;
                            _jobs.emplace_back(std::move(out));
                            ++_queued;
                        }
                    }
                }
            }

            wake(scheduled);

            lock.unlock();
        }

        --_total_jobs;

        job = std::move(next);
        index = 0;
    }

    return true;
}


}
}

