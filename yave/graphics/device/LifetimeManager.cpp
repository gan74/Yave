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
        const VkSemaphore timeline_semaphore = command_queue().timeline().vk_semaphore();

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
                collect_cmd_buffers();
            }
        }
    });
#endif
}

LifetimeManager::~LifetimeManager() {
    collect_cmd_buffers();

    y_debug_assert(_create_counter == _next_to_collect);
    y_always_assert(_in_flight.locked([](auto&& in_flight) { return in_flight.is_empty(); }), "CmdBuffer still in flight");

    _to_destroy.locked([&](auto&& to_destroy) {
        for(auto& res : to_destroy) {
            y_always_assert(res.first == _next_to_collect, "Resourse is still waiting on unsignaled fence");
            destroy_resource(res.second);
        }
    });
}

void LifetimeManager::shutdown_collector_thread() {
#ifdef YAVE_MT_LIFETIME_MANAGER
    y_profile();

    y_always_assert(_run_thread, "Collector thread not running");

    _run_thread = false;
    create_disposable_cmd_buffer().submit().wait();
    _collector_thread.join();
#endif
}



void LifetimeManager::register_pending(core::Span<CmdBufferData*> datas) {
    y_profile();

    bool collect = false;

    _in_flight.locked([&](auto&& in_flight) {
        in_flight.set_min_capacity(in_flight.size() + datas.size());
        for(CmdBufferData* data : datas) {
            y_debug_assert(data->timeline_fence().is_valid());
            y_debug_assert(std::find(in_flight.begin(), in_flight.end(), data) == in_flight.end());
            const auto it = std::lower_bound(in_flight.begin(), in_flight.end(), data, compare_cmd_buffers);
            in_flight.insert(it, data);
        }

        collect = (in_flight.first()->resource_fence()._value == _next_to_collect);
    });

    unused(collect);
#ifndef YAVE_MT_LIFETIME_MANAGER
    if(collect) {
        collect_cmd_buffers();
    }
#endif
}

void LifetimeManager::collect_cmd_buffers() {
    y_profile();

    // To ensure that CmdBufferData keep alives are freed outside the lock
    core::ScratchVector<CmdBufferData*> to_recycle;
    u64 next = 0;

    {
        y_profile_zone("collecting fences");
        _in_flight.locked([&](auto&& in_flight) {
            y_debug_assert(std::is_sorted(in_flight.begin(), in_flight.end(), compare_cmd_buffers));

            to_recycle = core::ScratchVector<CmdBufferData*>(in_flight.size());

            while(!in_flight.is_empty()) {
                CmdBufferData* data = in_flight.first();
                if(data->resource_fence()._value != _next_to_collect) {
                    break;
                }

                if(data->is_ready()) {
                    in_flight.pop_front();
                    to_recycle.emplace_back(data);
                    ++_next_to_collect;
                } else {
                    break;
                }
            }

            next = _next_to_collect;
        });
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

void LifetimeManager::wait_cmd_buffers() {
    y_profile();

    create_disposable_cmd_buffer().submit().wait();

    _in_flight.locked([&](auto&& in_flight) {
        for(CmdBufferData* data : in_flight) {
            data->timeline_fence().wait();
        }

        collect_cmd_buffers();

        y_debug_assert(in_flight.is_empty());
    });
}

void LifetimeManager::clear_resources(u64 up_to) {
    y_profile();

    core::SmallVector<ManagedResource, 64> to_delete;

    {
        y_profile_zone("collection");
        _to_destroy.locked([&](auto&& to_destroy) {
            y_debug_assert(std::is_sorted(to_destroy.begin(), to_destroy.end(), [](const auto& a, const auto& b) { return a.first < b.first; }));
            while(!to_destroy.is_empty() && to_destroy.first().first <= up_to) {
                to_delete.push_back(std::move(to_destroy.first().second));
                to_destroy.pop_front();
            }
        });
    }

    y_profile_zone("clear");
    for(auto& res : to_delete) {
        destroy_resource(res);
    }
}


ResourceFence LifetimeManager::create_fence() {
    return _create_counter++;
}

usize LifetimeManager::pending_cmd_buffers() const {
    return _in_flight.locked([](auto&& in_flight) { return in_flight.size(); });
}

usize LifetimeManager::pending_deletions() const {
    return _to_destroy.locked([](auto&& to_destroy) { return to_destroy.size(); });
}


void LifetimeManager::destroy_resource(ManagedResource& resource) const {
    std::visit(
        [](auto&& res) {
            if constexpr(std::is_same_v<decltype(res), EmptyResource&>) {
                y_fatal("Empty resource");
            } else if constexpr(std::is_same_v<decltype(res), DeviceMemory&>) {
                res.free();
            } else if constexpr(std::is_same_v<decltype(res), DescriptorSetData&>) {
                res.recycle();
            } else if constexpr(std::is_same_v<decltype(res), MeshDrawData&>) {
                res.recycle();
            } else if constexpr(std::is_same_v<decltype(res), MaterialDrawData&>) {
                res.recycle();
            } else {
                // log_msg(fmt("destroying {} {}", ct_type_name<decltype(res)>(), (void*)res));
                vk_destroy(res.consume());
            }
        },
        resource);
}


}
