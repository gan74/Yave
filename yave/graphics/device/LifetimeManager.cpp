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

#include <yave/graphics/utils.h>

#include <yave/graphics/commands/CmdBufferData.h>
#include <yave/graphics/commands/CmdBufferPool.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

static bool compare_cmd_buffers(CmdBufferData* a, CmdBufferData* b) {
    return a->resource_fence() < b->resource_fence();
}

LifetimeManager::LifetimeManager(DevicePtr dptr) : DeviceLinked(dptr) {
}

LifetimeManager::~LifetimeManager() {
    y_debug_assert(_create_counter == _next);

    for(auto& r : _to_destroy) {
        destroy_resource(r.second);
    }

    if(_fences.size()) {
        y_fatal("% fences still in flight.", _fences.size());
    }
}

ResourceFence LifetimeManager::create_fence() {
    return _create_counter++;
}

void LifetimeManager::register_for_polling(CmdBufferData* data) {
    if(data->is_signaled()) {
        release({data});
        return;
    }

    {
        const auto lock = y_profile_unique_lock(_cmd_lock);

        data->_timer.reset();

        const auto it = std::lower_bound(_in_flight.begin(), _in_flight.end(), data, compare_cmd_buffers);
        _in_flight.insert(it, data);

        y_debug_assert(std::is_sorted(_in_flight.begin(), _in_flight.end(), compare_cmd_buffers));
    }

    poll_cmd_buffers();
}


void LifetimeManager::poll_cmd_buffers() {
    y_profile();

    log_msg("Polling...");

    core::Vector<CmdBufferData*> to_release;
    {
        const auto lock = y_profile_unique_lock(_cmd_lock);

        usize j = 0;
        for(usize i = 0; i != _in_flight.size(); ++i) {
            _in_flight[i - j] = _in_flight[i];
            if(_in_flight[i]->_timer.elapsed().to_secs() < 1) {
                continue;
            }
            if(_in_flight[i]->poll_and_signal()) {
                to_release << _in_flight[i];
                log_msg(fmt("releasing after % ms", _in_flight[i]->_timer.elapsed().to_millis()));
                ++j;
            }
        }

        for(usize i = 0; i != j; ++i) {
            _in_flight.pop();
        }
    }

    release(to_release);
}


void LifetimeManager::release(core::Span<CmdBufferData*> buffers) {
    y_profile();

    if(buffers.is_empty()) {
        return;
    }

    y_debug_assert(std::is_sorted(buffers.begin(), buffers.end(), compare_cmd_buffers));

    u64 up_to = 0;
    bool run_collection = false;
    {
        const auto lock = y_profile_unique_lock(_fences_lock);
        const u64 prev = _next;

        usize to_skip = 0;
        while(to_skip < buffers.size() && buffers[to_skip]->resource_fence()._value == _next) {
            ++to_skip;
            ++_next;
        }

        if(to_skip != buffers.size()) {

            Y_TODO(Optimize)
            std::transform(buffers.begin() + to_skip, buffers.end(), std::back_inserter(_fences), [](CmdBufferData* data) { return data->resource_fence(); });
            std::sort(_fences.begin(), _fences.end());

            usize fence_index = 0;
            while(fence_index < _fences.size() && _fences[fence_index] == _next) {
                ++fence_index;
                ++_next;
            }

            if(fence_index) {
                for(usize i = 0; i != _fences.size() - fence_index; ++i) {
                    _fences[i] = _fences[i + fence_index];
                }
                for(usize i = 0; i != fence_index; ++i) {
                    _fences.pop();
                }
            }
        }

        up_to = _next;
        run_collection = prev != up_to;
    }

    for(CmdBufferData* data : buffers) {
        data->pool()->release(data);
    }

    if(run_collection) {
        clear_resources(up_to);
    }
}

usize LifetimeManager::pending_cmd_buffers() const {
    const auto lock = y_profile_unique_lock(_cmd_lock);
    return _in_flight.size();
}

usize LifetimeManager::pending_fences() const {
    const auto lock = y_profile_unique_lock(_fences_lock);
    return _fences.size();
}

usize LifetimeManager::pending_deletions() const {
    const auto lock = y_profile_unique_lock(_resources_lock);
    return _to_destroy.size();
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
        [dptr = device()](auto& res) {
            if constexpr(std::is_same_v<decltype(res), EmptyResource&>) {
                y_fatal("Empty resource");
            } else if constexpr(std::is_same_v<decltype(res), DeviceMemory&>) {
                res.free();
            } else if constexpr(std::is_same_v<decltype(res), DescriptorSetData&>) {
                res.recycle();
            } else {
                vk_destroy(dptr, res);
            }
        },
        resource);
}



#if 0

void LifetimeManager::queue_for_recycling(CmdBufferData* data) {
    y_profile();

    Y_TODO(check for data state)

    bool run_collect = false;
    {
        const auto lock = y_profile_unique_lock(_cmd_lock);

        const InFlightCmdBuffer in_flight = { data->resource_fence(), data };
        const auto it = std::lower_bound(_in_flight.begin(), _in_flight.end(), in_flight);
        _in_flight.insert(it, in_flight);

        run_collect = (_in_flight.front().fence._value == _done_counter + 1);
    }

    if(run_collect) {
        collect();
    }
}


void LifetimeManager::set_recycled(CmdBufferData* data) {
    y_profile();

    {
        const InFlightCmdBuffer cmd = { data->resource_fence(), data };

        const auto lock = y_profile_unique_lock(_cmd_lock);
        const auto it = std::lower_bound(_in_flight.begin(), _in_flight.end(), cmd);

        // Lookup may fail if the cmd buffer got recycled (from another thread) in between the poll_and_signal() and here
        if(it != _in_flight.end() && it->fence == cmd.fence) {
            it->data = nullptr;
        }
    }
}

void LifetimeManager::collect() {
    y_profile();

    u64 next = 0;
    bool clear = false;

    // To ensure that CmdBufferData keep alives are freed outside the lock
    core::Vector<CmdBufferData*> to_recycle;
    {
        y_profile_zone("fence polling");
        const auto lock = y_profile_unique_lock(_cmd_lock);

        y_debug_assert(std::is_sorted(_in_flight.begin(), _in_flight.end()));

        next = _done_counter;
        while(!_in_flight.empty()) {
            InFlightCmdBuffer& cmd = _in_flight.front();

            const u64 fence = cmd.fence._value;
            if(fence != next + 1) {
                break;
            }

            CmdBufferData* data = cmd.data;
            if(!data || data->poll_and_signal()) {
                next = fence;
                _in_flight.pop_front();

                if(data) {
                    to_recycle.emplace_back(data);
                }
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
        for(const auto& cmd : to_recycle) {
            cmd->pool()->recycle(cmd);
        }
    }

    if(clear) {
        clear_resources(next);
    }
}

#endif


}

