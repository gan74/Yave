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

#include <yave/graphics/device/LifetimeManager.h>
#include <y/core/ScratchPad.h>

namespace yave {

static VkHandle<VkSemaphore> create_cmd_buffer_semaphore() {
    VkHandle<VkSemaphore> semaphore;

    VkSemaphoreCreateInfo create_info = vk_struct();
    vk_check(vkCreateSemaphore(vk_device(), &create_info, vk_allocation_callbacks(), semaphore.get_ptr_for_init()));

    return semaphore;
}


WaitToken::WaitToken(const TimelineFence& fence) : _fence(fence) {
}

void WaitToken::wait() {
    wait_for_fence(_fence);
}



CmdQueue::CmdQueue(u32 family_index, VkQueue queue) : _queue(queue), _family_index(family_index) {
}

CmdQueue::~CmdQueue() {
    y_always_assert(_delayed_start.locked([&](auto&& delayed) { return delayed.is_empty(); }), "Delayed cmd buffers have not been flushed");
    wait();
}

u32 CmdQueue::family_index() const {
    return _family_index;
}

void CmdQueue::wait() {
    _queue.locked([](auto&& queue) {
        vk_check(vkQueueWaitIdle(queue));
    });
}

void CmdQueue::submit_async_delayed_start(TransferCmdBufferRecorder&& recorder) {
    vk_check(vkEndCommandBuffer(recorder.vk_cmd_buffer()));
    _delayed_start.locked([&](auto&& delayed) {
        delayed.emplace_back(std::exchange(recorder._data, nullptr));
    });
}

void CmdQueue::submit_async_delayed_start(ComputeCmdBufferRecorder&& recorder) {
    vk_check(vkEndCommandBuffer(recorder.vk_cmd_buffer()));
    _delayed_start.locked([&](auto&& delayed) {
        delayed.emplace_back(std::exchange(recorder._data, nullptr));
    });
}

WaitToken CmdQueue::submit(TransferCmdBufferRecorder&& recorder) {
    return submit_internal(std::move(recorder));
}

WaitToken CmdQueue::submit(ComputeCmdBufferRecorder&& recorder) {
    return submit_internal(std::move(recorder));
}

WaitToken CmdQueue::submit(CmdBufferRecorder&& recorder) {
    return submit_internal(std::move(recorder));
}

VkResult CmdQueue::present(CmdBufferRecorder&& recorder, const FrameToken& token, const Swapchain::FrameSyncObjects& swaphain_sync) {
    submit_internal(std::move(recorder), swaphain_sync.image_available, swaphain_sync.render_complete, swaphain_sync.fence);

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

WaitToken CmdQueue::submit_internal(CmdBufferRecorderBase&& recorder, VkSemaphore wait, VkSemaphore signal, VkFence fence) {
    y_profile();

    const VkCommandBuffer cmd_buffer = recorder.vk_cmd_buffer();
    vk_check(vkEndCommandBuffer(cmd_buffer));

    TimelineFence timeline_fence;
    const VkSemaphore timeline_semaphore = vk_timeline_semaphore();

    core::SmallVector<CmdBufferData*> pending;

    _queue.locked([&](auto&& queue) {
        // This needs to be inside the lock
        timeline_fence = create_timeline_fence();
        const u64 prev_value = timeline_fence._value - 1;
        recorder._data->_timeline_fence = timeline_fence;

        core::SmallVector<VkSubmitInfo> submit_infos;
        core::SmallVector<VkSemaphore> wait_semaphores;
        core::SmallVector<u64> wait_values;

        _delayed_start.locked([&](auto&& delayed) {
            for(CmdBufferData* data : delayed) {
                if(!data->_semaphore) {
                    data->_semaphore = create_cmd_buffer_semaphore();
                }

                VkSubmitInfo& submit_info = submit_infos.emplace_back<VkSubmitInfo>(vk_struct());
                {
                    submit_info.commandBufferCount = 1;
                    submit_info.pCommandBuffers = &data->_cmd_buffer;
                    submit_info.signalSemaphoreCount = 1;
                    submit_info.pSignalSemaphores = &data->_semaphore.get();
                }

                wait_semaphores.push_back(data->_semaphore.get());
                wait_values.push_back(0);

                data->_timeline_fence = timeline_fence;
                pending << data;
            }

            delayed.make_empty();
        });


        {
            wait_semaphores.push_back(timeline_semaphore);
            wait_values.push_back(prev_value);

            if(wait) {
                wait_semaphores.push_back(wait);
                wait_values.push_back(0);
            }
        }

        y_debug_assert(wait_semaphores.size() == wait_values.size());

        const std::array<u64, 2> signal_values = {timeline_fence._value, 0};
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

        VkSubmitInfo& submit_info = submit_infos.emplace_back<VkSubmitInfo>(vk_struct());
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
        vk_check(vkQueueSubmit(queue, u32(submit_infos.size()), submit_infos.data(), fence));
    });

    pending << std::exchange(recorder._data, nullptr);
    lifetime_manager().register_pending(pending);

    return WaitToken(timeline_fence);
}

}

