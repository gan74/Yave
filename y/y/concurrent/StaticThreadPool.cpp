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

#include "StaticThreadPool.h"
#include "concurrent.h"

#include <y/core/Chrono.h>
#include <y/utils/format.h>

namespace y {
namespace concurrent {

bool DependencyGroup::Data::is_ready() const {
    return counter == max;
}

bool DependencyGroup::Data::notify() {
    y_debug_assert(counter < max);
    return (++counter) == max;
}

DependencyGroup::DependencyGroup() {
}

void DependencyGroup::reset() {
    if(_data) {
        y_always_assert(_data->is_ready(), "Dependency group is not ready");
        _data->max = 0;
        _data->counter = 0;
    }
}

void DependencyGroup::init() {
    if(!_data) {
        _data = std::make_shared<Data>();
    }
}

bool DependencyGroup::is_empty() const {
    return !_data;
}

bool DependencyGroup::is_ready() const {
    return !_data || _data->is_ready();
}

std::shared_ptr<DependencyGroup::Data> DependencyGroup::create_signal() {
    init();

    y_debug_assert(!_data->counter);
    ++(_data->max);

    return _data;
}


StaticThreadPool::Task::Task(Func func, core::Span<DependencyGroup> wait, std::shared_ptr<DependencyGroup::Data> sig, std::source_location loc) :
        function(std::move(func)),
        signal(std::move(sig)) {

    std::copy_if(wait.begin(), wait.end(), std::back_inserter(wait_for), [](const auto& dep) { return !dep.is_empty(); });

#ifdef Y_DEBUG
    location = loc;
#endif
}

bool StaticThreadPool::Task::is_ready() const {
    return std::all_of(wait_for.begin(), wait_for.end(), [](const auto& dg) { return dg.is_ready(); });
}




StaticThreadPool::StaticThreadPool(usize thread_count) {
    for(usize i = 0; i != thread_count; ++i) {
        _threads.emplace_back([this, i] {
            concurrent::set_thread_name(fmt_c_str("Worker thread #{}", i));
            worker();
        });
    }
}

StaticThreadPool::~StaticThreadPool() {
    while(process_one(std::unique_lock(_lock))) {
        // Nothing
    }

    {
        _run = false;
        ++_generation;
        const std::unique_lock lock(_lock);
        _condition.notify_all();
    }

    for(auto& thread : _threads) {
        thread.join();
    }

    y_debug_assert(is_empty());
}

usize StaticThreadPool::concurency() const {
    return _threads.size();
}

bool StaticThreadPool::is_empty() const {
    const std::unique_lock lock(_lock);
    return _queue.is_empty() && !_working_threads;
}

usize StaticThreadPool::pending_tasks() const {
    const std::unique_lock lock(_lock);
    return _queue.size() + _working_threads;
}

void StaticThreadPool::cancel_pending_tasks() {
    const std::unique_lock lock(_lock);
    _queue.clear();
}

void StaticThreadPool::process_until_complete(core::Span<DependencyGroup> wait_for) {
    auto is_done = [&] { return std::all_of(wait_for.begin(), wait_for.end(), [](const DependencyGroup& d) { return d.is_ready(); }); };

    while(!is_done()) {
        if(process_one(std::unique_lock(_lock))) {
            continue;
        }

        std::unique_lock lock(_lock);
        if(is_done()) {
            break;
        }

        const u64 gen = _generation;
        _condition.wait(lock, [&] { return gen != _generation; });
    }
}

void StaticThreadPool::schedule(Func&& func, DependencyGroup* signal, core::Span<DependencyGroup> wait_for, std::source_location loc) {
    y_debug_assert(_run);
    y_debug_assert(std::none_of(wait_for.begin(), wait_for.end(), [](const auto& d) { return d.is_empty(); }));

    std::shared_ptr<DependencyGroup::Data> signal_data;
    if(signal) {
        signal_data = signal->create_signal();
    }

    {
        const std::unique_lock lock(_lock);

        auto& task = _queue.emplace_back();
        task = std::make_shared<Task>(std::move(func), wait_for, std::move(signal_data), loc);
    }

    if(concurency()) {
        ++_generation;
        _condition.notify_one();
    } else {
        while(process_one(std::unique_lock(_lock))) {
            // Nothing
        }
    }
}

bool StaticThreadPool::process_one(std::unique_lock<std::mutex> lock) {
    ++_working_threads;
    y_defer(--_working_threads);

    for(auto it = _queue.begin(); it != _queue.end(); ++it) {
        if((*it)->is_ready()) {
            auto task = std::move(*it);
            _queue.erase(it);

            lock.unlock();

            task->function();

            if(task->signal) {
                lock.lock();
                if(task->signal->notify()) {
                    ++_generation;
                    _condition.notify_all();
                }
            }

            return true;
        }
    }

    // We must not unlock if we don't do anything
    return false;
}

void StaticThreadPool::worker() {
    while(_run) {
        std::unique_lock lock(_lock);
        const u64 gen = _generation;
        _condition.wait(lock, [&] { return (!_queue.is_empty() || !_run) && gen != _generation; });
        process_one(std::move(lock));
    }
}

}
}

