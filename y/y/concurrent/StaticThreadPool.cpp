/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

bool DependencyGroup::is_empty() const {
    return _counter == nullptr;
}

bool DependencyGroup::is_ready() const {
    return dependency_count() == 0;
}

bool DependencyGroup::is_expired() const {
    return _counter != nullptr && (*_counter) == 0;
}

u32 DependencyGroup::dependency_count() const {
    return !_counter ? u32(0) : u32(*_counter);
}

void DependencyGroup::add_dependency() {
    if(!_counter) {
        _counter = std::make_shared<std::atomic<u32>>(1);
    } else {
        ++(*_counter);
    }
}

void DependencyGroup::solve_dependency() {
    if(_counter) {
        y_debug_assert(*_counter != 0); // not 100% thread safe but we don't care
        --(*_counter);
    }
}


StaticThreadPool::FuncData::FuncData(Func func, core::Span<DependencyGroup> wait, DependencyGroup done) :
        function(std::move(func)),
        wait_for(wait),
        on_done(std::move(done)) {
}

bool StaticThreadPool::FuncData::is_ready() const {
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
    process_until_empty();

    {
        _shared_data.run = false;
        const std::unique_lock lock(_shared_data.lock);
        _shared_data.condition.notify_all();
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
    const std::unique_lock lock(_shared_data.lock);
    return _shared_data.queue.empty() && !_shared_data.working;
}

usize StaticThreadPool::pending_tasks() const {
    const std::unique_lock lock(_shared_data.lock);
    return _shared_data.queue.size() + _shared_data.working;
}

void StaticThreadPool::cancel_pending_tasks() {
    const std::unique_lock lock(_shared_data.lock);
    _shared_data.queue.clear();
}

void StaticThreadPool::process_until_empty() {
    while(true) {
        std::unique_lock lock(_shared_data.lock);
        if(!process_one(std::move(lock))) {
            break;
        }
        // Nothing
    }
}

void StaticThreadPool::schedule(Func&& func, DependencyGroup* on_done, core::Span<DependencyGroup> wait_for) {
    y_debug_assert(_shared_data.run);

    {
        const std::unique_lock lock(_shared_data.lock);
        if(on_done) {
            on_done->add_dependency();
            _shared_data.queue.emplace_back(std::move(func), wait_for, *on_done);
        } else {
            _shared_data.queue.emplace_back(std::move(func), wait_for);
        }
    }

    if(concurency()) {
        _shared_data.condition.notify_one();
    } else {
        process_until_empty();
    }
}

bool StaticThreadPool::process_one(std::unique_lock<std::mutex> lock) {
    ++_shared_data.working;
    y_defer(--_shared_data.working);

    for(auto it = _shared_data.queue.begin(); it != _shared_data.queue.end(); ++it) {
        if(it->is_ready()) {
            auto f = std::move(*it);
            _shared_data.queue.erase(it);

            lock.unlock();

            f.function();

            {
                f.on_done.solve_dependency();
                if(f.on_done.is_ready()) {
                    _shared_data.condition.notify_one();
                }
            }
            return true;
        }
    }

    return false;
}

void StaticThreadPool::worker() {
    while(_shared_data.run) {
        std::unique_lock lock(_shared_data.lock);
        _shared_data.condition.wait(lock, [&] { return !_shared_data.queue.empty() || !_shared_data.run; });
        process_one(std::move(lock));
    }
}

}
}

