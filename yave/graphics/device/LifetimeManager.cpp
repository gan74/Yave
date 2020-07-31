/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include "LifetimeManager.h"
#include "destroy.h"

#include <yave/graphics/utils.h>

#include <yave/graphics/commands/CmdBufferData.h>
#include <yave/graphics/commands/CmdBufferPool.h>


#include <y/concurrent/concurrent.h>
#include <y/core/Chrono.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

LifetimeManager::LifetimeManager(DevicePtr dptr) : DeviceLinked(dptr) {
#ifdef YAVE_ASYNC_RESOURCE_COLLECTION
    _run_async = true;
    _collect_thread = std::make_unique<std::thread>([this] {
        concurrent::set_thread_name("Resource collection thread");
        std::mutex mutex;
        while(_run_async) {
            auto lock = y_profile_unique_lock(mutex);
            const u32 ms = _collection_interval;
            if(ms) {
                _collect_condition.wait_for(lock, std::chrono::milliseconds(ms));
            } else {
                _collect_condition.wait(lock);
            }
            collect();
        }
    });
#endif
}

LifetimeManager::~LifetimeManager() {
    y_debug_assert(_counter == _done_counter);

    stop_async_collection();
    collect();

    for(auto& r : _to_destroy) {
        destroy_resource(r.second);
    }

    if(_in_flight.size()) {
        y_fatal("% CmdBuffer still in flight.", _in_flight.size());
    }
}

void LifetimeManager::stop_async_collection() {
#ifdef YAVE_ASYNC_RESOURCE_COLLECTION
    if(is_async()) {
        y_profile();
        _run_async = false;
        _collect_condition.notify_all();
        _collect_thread->join();
        _collect_thread = nullptr;
    }
#endif
}

bool LifetimeManager::is_async() const {
#ifdef YAVE_ASYNC_RESOURCE_COLLECTION
    return _run_async;
#else
    return false;
#endif
}

void LifetimeManager::schedule_collection() {
#ifdef YAVE_ASYNC_RESOURCE_COLLECTION
    if(is_async()) {
        y_profile_event();
        _collect_condition.notify_all();
    } else {
        collect();
    }
#else
    collect();
#endif
}


ResourceFence LifetimeManager::create_fence() {
    return ++_counter;
}

void LifetimeManager::recycle(CmdBufferData&& cmd) {
    y_profile();

    bool run_collect = false;
    {
        const auto lock = y_profile_unique_lock(_cmd_lock);
        const auto it = std::lower_bound(_in_flight.begin(), _in_flight.end(), cmd,
                                   [](const auto& a, const auto& b) { return a.resource_fence() < b.resource_fence(); });
        _in_flight.insert(it, std::move(cmd));
        run_collect = (_in_flight.front().resource_fence()._value == _done_counter + 1);
    }

    if(run_collect) {
        schedule_collection();
    }
}

void LifetimeManager::collect() {
    y_profile();

    u64 next = 0;
    bool clear = false;

    // To ensure that CmdBufferData keep alives are freed outside the lock
    core::Vector<CmdBufferData> to_clean;
    {
        y_profile_zone("fence polling");
        const auto lock = y_profile_unique_lock(_cmd_lock);
        next = _done_counter;
        while(!_in_flight.empty()) {
            CmdBufferData& cmd = _in_flight.front();
            const u64 fence = cmd.resource_fence()._value;
            if(fence != next + 1) {
                break;
            }

            if(vkGetFenceStatus(vk_device(device()), cmd.vk_fence()) == VK_SUCCESS) {
                next = fence;
                to_clean.emplace_back(std::move(_in_flight.front()));
                _in_flight.pop_front();
            } else {
                break;
            }
        }

        if(next != _done_counter) {
            clear = true;
            _done_counter = next;
        }
    }

    {
        y_profile_zone("release");
        for(auto& cmd : to_clean) {
            if(cmd.pool()) {
                cmd.pool()->release(std::move(cmd));
            }
        }
    }

    if(clear) {
        clear_resources(next);
    }
}

usize LifetimeManager::pending_deletions() const {
    const std::unique_lock lock(_resource_lock);
    return _to_destroy.size();
}

usize LifetimeManager::active_cmd_buffers() const {
    const auto lock = y_profile_unique_lock(_cmd_lock);
    return _in_flight.size();
}

void LifetimeManager::destroy_resource(ManagedResource& resource) const {
    std::visit(
        [dptr = device()](auto& res) {
            if constexpr(std::is_same_v<decltype(res), EmptyResource&>) {
                y_fatal("Empty resource");
            } else if constexpr(std::is_same_v<decltype(res), DeviceMemory&>) {
                y_profile_zone("free");
                res.free();
            } else if constexpr(std::is_same_v<decltype(res), DescriptorSetData&>) {
                y_profile_zone("recycle");
                res.recycle();
            } else {
                y_profile_zone("destroy");
                vk_destroy(dptr, res);
            }
        },
        resource);
}

void LifetimeManager::clear_resources(u64 up_to) {
    y_profile();
    core::Vector<ManagedResource> to_del;

    {
        y_profile_zone("collection");
        const auto lock = y_profile_unique_lock(_cmd_lock);
        while(!_to_destroy.empty() && _to_destroy.front().first <= up_to) {
            to_del << std::move(_to_destroy.front().second);
            _to_destroy.pop_front();
        }
    }

    y_profile_zone("clear");
    for(auto& res : to_del) {
        destroy_resource(res);
    }
}

}

