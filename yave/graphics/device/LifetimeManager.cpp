/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <y/utils/format.h>

namespace yave {

static bool compare_cmd_buffers(CmdBufferData* a, CmdBufferData* b) {
    return a->resource_fence() < b->resource_fence();
}

LifetimeManager::LifetimeManager() {
}

LifetimeManager::~LifetimeManager() {
    y_debug_assert(_create_counter == _next);

    wait_cmd_buffers();

    y_always_assert(_in_flight.empty(), "CmdBuffer still in flight");

    for(auto& res : _to_destroy) {
        y_always_assert(res.first == _next, "Resourse is still waiting on unsignaled fence.");
        destroy_resource(res.second);
    }
}

void LifetimeManager::wait_cmd_buffers() {
    y_profile();

    const auto lock = y_profile_unique_lock(_cmd_lock);

    for(CmdBufferData* data : _in_flight) {
        data->wait();
    }

    poll_cmd_buffers();
}


ResourceFence LifetimeManager::create_fence() {
    return _create_counter++;
}

void LifetimeManager::register_for_polling(CmdBufferData* data) {
    bool collect = false;

    {
        const auto lock = y_profile_unique_lock(_cmd_lock);

        const auto it = std::lower_bound(_in_flight.begin(), _in_flight.end(), data, compare_cmd_buffers);
        _in_flight.insert(it, data);

        collect = (_in_flight.front()->resource_fence()._value == _next);
    }

    if(collect) {
        poll_cmd_buffers();
    }
}

void LifetimeManager::poll_cmd_buffers() {
    y_profile();

    u64 next = 0;
    bool clear = false;

    // To ensure that CmdBufferData keep alives are freed outside the lock
    core::Vector<CmdBufferData*> to_recycle;
    {
        y_profile_zone("fence polling");
        const auto lock = y_profile_unique_lock(_cmd_lock);

        y_debug_assert(std::is_sorted(_in_flight.begin(), _in_flight.end(), compare_cmd_buffers));

        next = _next;
        while(!_in_flight.empty()) {
            CmdBufferData* data = _in_flight.front();
            if(data->resource_fence()._value != next) {
                break;
            }

            if(data->poll_and_signal()) {
                _in_flight.pop_front();
                ++next;

                to_recycle.emplace_back(data);
            } else {
                break;
            }
        }

        clear = next != _next;
        _next = next;
    }

    {
        y_profile_zone("release");
        for(CmdBufferData* data : to_recycle) {
            data->pool()->release(data);
        }
    }

    if(clear) {
        clear_resources(next);
    }
}

void LifetimeManager::clear_resources(u64 up_to) {
    y_profile();

    core::Vector<ManagedResource> to_delete;

    {
        y_profile_zone("collection");
        const auto lock = y_profile_unique_lock(_resources_lock);
        while(!_to_destroy.empty() && _to_destroy.front().first < up_to) {
            to_delete << std::move(_to_destroy.front().second);
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
            } else {
                vk_destroy(res);
            }
        },
        resource);
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
