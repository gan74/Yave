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

#include "LifetimeManager.h"
#include "destroy.h"

#include <yave/graphics/graphics.h>

#include <yave/graphics/commands/CmdBufferData.h>
#include <yave/graphics/commands/CmdBufferPool.h>
#include <yave/graphics/commands/CmdQueue.h>

#include <y/core/ScratchPad.h>
#include <y/concurrent/concurrent.h>
#include <y/utils/format.h>
#include <y/utils/log.h>

namespace yave {

static bool compare_cmd_buffers(const CmdBufferData* a, const CmdBufferData* b) {
    return a->resource_fence() < b->resource_fence();
}

LifetimeManager::LifetimeManager() {
#ifdef YAVE_MT_LIFETIME_MANAGER
    _collector_thread = std::thread([this] {
        concurrent::set_thread_name("LifetimeManager collector thread");
        const VkDevice device = vk_device();
        const VkSemaphore timeline_semaphore = vk_timeline_semaphore();

        u64 semaphore_value = 0;
        vk_check(vkGetSemaphoreCounterValue(device, timeline_semaphore, &semaphore_value));

        while(_run_thread) {
            ++semaphore_value;
            VkSemaphoreWaitInfo wait_info = vk_struct();
            {
                wait_info.pSemaphores = &timeline_semaphore;
                wait_info.pValues = &semaphore_value;
                wait_info.semaphoreCount = 1;
            }

            const VkResult result = vkWaitSemaphores(device, &wait_info, u64(100'000'000)); // 100ms
            vk_check(result);

            if(result == VK_TIMEOUT) {
                --semaphore_value;
                // Very spammy, especially if the window is hidden
                /*if(_run_thread) {
                    log_msg("Semaphore was not signaled after 100ms", Log::Warning);
                }*/
            } else {
                poll_cmd_buffers();
            }
        }
    });
#endif
}

LifetimeManager::~LifetimeManager() {
    poll_cmd_buffers();

    y_debug_assert(_create_counter == _next);
    y_always_assert(_in_flight.empty(), "CmdBuffer still in flight");

    for(auto& res : _to_destroy) {
        y_always_assert(res.first == _next, "Resourse is still waiting on unsignaled fence.");
        destroy_resource(res.second);
    }
}

void LifetimeManager::shutdown_collector_thread() {
#ifdef YAVE_MT_LIFETIME_MANAGER
    y_profile();

    y_always_assert(_run_thread, "Collector thread not running");

    _run_thread = false;
    command_queue().submit(create_disposable_cmd_buffer()).wait();
    _collector_thread.join();
#endif
}

void LifetimeManager::poll_cmd_buffers() {
    y_profile();

    // To ensure that CmdBufferData keep alives are freed outside the lock
    core::ScratchVector<CmdBufferData*> to_recycle;
    u64 next = 0;

    {
        y_profile_zone("fence polling");
        const auto lock = y_profile_unique_lock(_cmd_lock);

        y_debug_assert(std::is_sorted(_in_flight.begin(), _in_flight.end(), compare_cmd_buffers));

        to_recycle = core::ScratchVector<CmdBufferData*>(_in_flight.size());

        next = _next;
        while(!_in_flight.empty()) {
            CmdBufferData* data = _in_flight.front();
            if(data->resource_fence()._value != next) {
                break;
            }

            if(data->poll()) {
                _in_flight.pop_front();
                ++next;

                to_recycle.emplace_back(data);
            } else {
                break;
            }
        }

        _next = next;
    }

    if(!to_recycle.is_empty()) {
        clear_resources(next);
    }

    {
        y_profile_zone("release");
        for(CmdBufferData* data : to_recycle) {
            data->pool()->release(data);
        }
    }
}

void LifetimeManager::clear_resources(u64 up_to) {
    y_profile();

    core::Vector<ManagedResource> to_delete;

    {
        y_profile_zone("collection");
        const auto lock = y_profile_unique_lock(_resources_lock);

        y_debug_assert(std::is_sorted(_to_destroy.begin(), _to_destroy.end(), [](const auto& a, const auto& b) { return a.first < b.first; }));
        while(!_to_destroy.empty() && _to_destroy.front().first <= up_to) {
            to_delete.push_back(std::move(_to_destroy.front().second));
            _to_destroy.pop_front();
        }
    }

    y_profile_zone("clear");
    for(auto& res : to_delete) {
        destroy_resource(res);
    }
}

void LifetimeManager::destroy_resource(ManagedResource& resource) const {
    std::visit(
        [](auto& res) {
            if constexpr(std::is_same_v<decltype(res), EmptyResource&>) {
                y_fatal("Empty resource");
            } else if constexpr(std::is_same_v<decltype(res), DeviceMemory&>) {
                res.free();
            } else if constexpr(std::is_same_v<decltype(res), DescriptorSetData&>) {
                res.recycle();
            } else if constexpr(std::is_same_v<decltype(res), MeshDrawData&>) {
                res.recycle();
            } else {
                // log_msg(fmt("destroying % %", ct_type_name<decltype(res)>(), (void*)res));
                vk_destroy(res.consume());
            }
        },
        resource);
}





void LifetimeManager::wait_cmd_buffers() {
    y_profile();

    const auto lock = y_profile_unique_lock(_cmd_lock);
    for(CmdBufferData* data : _in_flight) {
        data->wait();
    }

    poll_cmd_buffers();

    y_debug_assert(_in_flight.empty());
}

ResourceFence LifetimeManager::create_fence() {
    return _create_counter++;
}

void LifetimeManager::register_for_polling(CmdBufferData* data) {
    bool collect = false;

    {
        const auto lock = y_profile_unique_lock(_cmd_lock);

        y_debug_assert(std::find(_in_flight.begin(), _in_flight.end(), data) == _in_flight.end());

        const auto it = std::lower_bound(_in_flight.begin(), _in_flight.end(), data, compare_cmd_buffers);
        _in_flight.insert(it, data);

        collect = (_in_flight.front()->resource_fence()._value == _next);
    }

    unused(collect);
#ifndef YAVE_MT_LIFETIME_MANAGER
    if(collect) {
        poll_cmd_buffers();
    }
#endif
}

usize LifetimeManager::pending_cmd_buffers() const {
    const auto lock = y_profile_unique_lock(_cmd_lock);
    return _in_flight.size();
}

usize LifetimeManager::pending_deletions() const {
    const auto lock = y_profile_unique_lock(_resources_lock);
    return _to_destroy.size();
}

}
