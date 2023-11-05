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

#include "CmdQueue.h"

#include <yave/graphics/commands/CmdBufferPool.h>
#include <yave/graphics/device/extensions/DebugUtils.h>
#include <yave/graphics/device/LifetimeManager.h>
#include <y/core/ScratchPad.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

static VkHandle<VkSemaphore> create_cmd_buffer_semaphore() {
    VkHandle<VkSemaphore> semaphore;

    VkSemaphoreCreateInfo create_info = vk_struct();
    vk_check(vkCreateSemaphore(vk_device(), &create_info, vk_allocation_callbacks(), semaphore.get_ptr_for_init()));

    return semaphore;
}

#ifdef YAVE_GPU_PROFILING
static auto create_profiling_ctx(VkQueue queue, u32 family_index) {
    y_profile();

    VkCommandPool pool = {};
    VkCommandBuffer cmd_buffer = {};

    {
        VkCommandPoolCreateInfo create_info = vk_struct();
        create_info.queueFamilyIndex = family_index;
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vk_check(vkCreateCommandPool(vk_device(), &create_info, vk_allocation_callbacks(), &pool));
    }

    {
        VkCommandBufferAllocateInfo allocate_info = vk_struct();
        allocate_info.commandBufferCount = 1;
        allocate_info.commandPool = pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vk_check(vkAllocateCommandBuffers(vk_device(), &allocate_info, &cmd_buffer));
    }

    const TracyVkCtx ctx = TracyVkContext(vk_physical_device(), vk_device(), queue, cmd_buffer);

    vkDestroyCommandPool(vk_device(), pool, vk_allocation_callbacks());

    return ctx;
}
#endif



concurrent::Mutexed<core::Vector<CmdQueue*>> CmdQueue::_all_queues = {};

CmdQueue::CmdQueue(u32 family_index, VkQueue queue) : _queue(queue), _family_index(family_index){
    _all_queues.locked([&](auto&& all_queues) {
        y_debug_assert(std::find(all_queues.begin(), all_queues.end(), this) == all_queues.end());
        all_queues << this;
    });

    _async_submit_data.locked([&](auto&& submit_data) {
        submit_data.current_fence = _timeline.current_timeline();
        submit_data.next_fence = _timeline.advance_timeline();
    });

#ifdef Y_DEBUG
    if(const auto* debug = debug_utils()) {
        debug->set_resource_name(queue, "Main queue");
    }
#endif

#ifdef YAVE_GPU_PROFILING
    _profiling_ctx = create_profiling_ctx(queue, family_index);
#endif
}

CmdQueue::~CmdQueue() {
    wait();

    // "Fake" submit to set consume _async_submit_data.next_fence
    _queue.locked([&](auto&& queue) {
        const TimelineFence next_fence = _async_submit_data.locked([](auto&& submit_data) { return submit_data.next_fence; });
        const VkSemaphore signal_sem = _timeline.vk_semaphore();
        const u64 signal_value = next_fence.value();

        VkTimelineSemaphoreSubmitInfo timeline_info = vk_struct();
        {
            timeline_info.signalSemaphoreValueCount = 1;
            timeline_info.pSignalSemaphoreValues = &signal_value;
        }

        VkSubmitInfo submit_info = vk_struct();
        {
            submit_info.pNext = &timeline_info;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &signal_sem;
        }

        vk_check(vkQueueSubmit(queue, 1, &submit_info, {}));
        vk_check(vkQueueWaitIdle(queue));
    });

    _all_queues.locked([&](auto&& all_queues) {
        const auto it = std::find(all_queues.begin(), all_queues.end(), this);
        y_debug_assert(it != all_queues.end());
        all_queues.erase_unordered(it);
    });

#ifdef YAVE_GPU_PROFILING
    TracyVkDestroy(_profiling_ctx);
#endif
}

u32 CmdQueue::family_index() const {
    return _family_index;
}

const Timeline& CmdQueue::timeline() const {
    return _timeline;
}

#ifdef YAVE_GPU_PROFILING
TracyVkCtx CmdQueue::profiling_context() const {
    return _profiling_ctx;
}
#endif

void CmdQueue::wait() {
    y_profile();

    _queue.locked([&](auto&& queue) {
        vk_check(vkQueueWaitIdle(queue));
    });
}

void CmdQueue::clear_all_cmd_pools() {
    y_profile();

    wait();
    _cmd_pools.locked([&](auto&& cmd_pools) {
        cmd_pools.clear();
    });
}

void CmdQueue::submit_async_start(CmdBufferData* data) {
    if(!data->_semaphore) {
        data->_semaphore = create_cmd_buffer_semaphore();
    }

    submit_internal(data, {}, {}, {}, true);
}

TimelineFence CmdQueue::submit(CmdBufferData* data) {
    return submit_internal(data);
}


VkResult CmdQueue::present(CmdBufferRecorder&& recorder, const FrameToken& token, const Swapchain::FrameSyncObjects& swaphain_sync) {
    y_profile();

    submit_internal(std::exchange(recorder._data, nullptr), swaphain_sync.image_available, swaphain_sync.render_complete, swaphain_sync.fence);

    return _queue.locked([&](auto&& queue) {
        y_profile_zone("present");
        VkPresentInfoKHR present_info = vk_struct();
        {
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &token.swapchain;
            present_info.pImageIndices = &token.image_index;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &swaphain_sync.render_complete.get();
        }

        return vkQueuePresentKHR(queue, &present_info);
    });
}

TimelineFence CmdQueue::submit_internal(CmdBufferData* data, VkSemaphore wait, VkSemaphore signal, VkFence fence, bool async_start) {
    y_profile();
    y_always_assert(!data->is_secondary(), "Secondaries can not be submitted directly");

    const VkCommandBuffer cmd_buffer = data->vk_cmd_buffer();

#ifdef YAVE_GPU_PROFILING
    TracyVkCollect(_profiling_ctx, cmd_buffer);
#endif

    vk_check(vkEndCommandBuffer(cmd_buffer));

    TimelineFence next_fence;
    core::SmallVector<CmdBufferData*> pending;
    if(async_start) {
        y_debug_assert(data->_semaphore);
        y_always_assert(!wait && !signal && !fence, "Invalid submit");

        _queue.locked([&](auto&& queue) {
            _async_submit_data.locked([&](auto&& submit_data) {
                const VkSemaphore semaphore = data->_semaphore;
                submit_data.semaphores << semaphore;

                VkSubmitInfo submit_info = vk_struct();
                {
                    submit_info.commandBufferCount = 1;
                    submit_info.pCommandBuffers = &cmd_buffer;
                    submit_info.signalSemaphoreCount = 1;
                    submit_info.pSignalSemaphores = &semaphore;
                }

                next_fence = submit_data.next_fence;

                y_profile_zone("submit async");
                vk_check(vkQueueSubmit(queue, 1, &submit_info, fence));
            });
        });
    } else {
        _queue.locked([&](auto&& queue) {
            core::Vector<VkSemaphore> wait_semaphores;
            TimelineFence current_fence;

            _async_submit_data.locked([&](auto&& submit_data) {
                current_fence = submit_data.current_fence;
                next_fence = submit_data.next_fence;
                submit_data.current_fence = _timeline.current_timeline();
                submit_data.next_fence = _timeline.advance_timeline();
                wait_semaphores.swap(submit_data.semaphores);
            });

            core::SmallVector<u64> wait_values;
            wait_values.set_min_size(wait_semaphores.size(), u64(0));

            const VkSemaphore timeline_semaphore = _timeline.vk_semaphore();

            {
                if(wait) {
                    wait_semaphores.push_back(wait);
                    wait_values.push_back(0);
                }

                wait_semaphores.push_back(timeline_semaphore);
                wait_values.push_back(current_fence.value());
            }

            y_debug_assert(current_fence.value() + 1 == next_fence.value());
            y_debug_assert(wait_semaphores.size() == wait_values.size());


            const std::array<u64, 2> signal_values = {next_fence.value(), 0};
            const std::array<VkSemaphore, 2> signal_semaphores = {timeline_semaphore, signal};
            const u32 signal_count = signal_semaphores[1] ? 2 : 1;
            const u32 wait_count = u32(wait_semaphores.size());

            const core::ScratchPad<VkPipelineStageFlags> wait_stages(wait_count, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

            VkTimelineSemaphoreSubmitInfo timeline_info = vk_struct();
            {
                y_debug_assert(wait_semaphores.size() == wait_values.size());
                timeline_info.waitSemaphoreValueCount = wait_count;
                timeline_info.pWaitSemaphoreValues = wait_values.data();
                timeline_info.signalSemaphoreValueCount = signal_count;
                timeline_info.pSignalSemaphoreValues = signal_values.data();
            }

            VkSubmitInfo submit_info = vk_struct();
            {
                submit_info.commandBufferCount = 1;
                submit_info.pCommandBuffers = &cmd_buffer;
                submit_info.pNext = &timeline_info;
                submit_info.pWaitDstStageMask = wait_stages.data();
                submit_info.waitSemaphoreCount = wait_count;
                submit_info.pWaitSemaphores = wait_semaphores.data();
                submit_info.signalSemaphoreCount = signal_count;
                submit_info.pSignalSemaphores = signal_semaphores.data();
            }

            y_profile_zone("submit");
            vk_check(vkQueueSubmit(queue, 1, &submit_info, fence));
        });
    }

    data->_timeline_fence = next_fence;
    pending << data;

    {
        y_profile_zone("push secondaries");
        for(usize i = 0, size = pending.size(); i != size; ++i) {
            for(CmdBufferData* secondary : pending[i]->_secondaries) {
                y_debug_assert(secondary->is_secondary());
                y_debug_assert(!secondary->_timeline_fence.is_valid());

                secondary->_timeline_fence = next_fence;
                pending << secondary;
            }
            pending[i]->_secondaries.make_empty();
        }
    }

    lifetime_manager().register_pending(pending);


    return next_fence;
}


CmdBufferPool& CmdQueue::cmd_pool_for_thread() {
    y_profile();

    static thread_local y_defer(
        const u32 thread_id = concurrent::thread_id();
        _all_queues.locked([&](auto&& all_queues) {
            for(CmdQueue* queue : all_queues) {
                queue->clear_thread(thread_id);
            }
        });
    );

    const u32 thread_id = concurrent::thread_id();
    return _cmd_pools.locked([&](auto&& cmd_pools) -> CmdBufferPool& {
        const auto it = std::find_if(cmd_pools.begin(), cmd_pools.end(), [=](const auto& pool) { return pool.first == thread_id; });
        if(it != cmd_pools.end()) {
            return *(it->second);
        }
        return *cmd_pools.emplace_back(thread_id, std::make_unique<CmdBufferPool>(this)).second;
    });
}

void CmdQueue::clear_thread(u32 thread_id) {
    y_profile();

    _cmd_pools.locked([&](auto&& cmd_pools) {
        const auto it = std::find_if(cmd_pools.begin(), cmd_pools.end(), [=](const auto& pool) { return pool.first == thread_id; });
        if(it != cmd_pools.end()) {
            cmd_pools.erase_unordered(it);
        }
    });
}

}

