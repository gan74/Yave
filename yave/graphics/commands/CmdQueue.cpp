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

namespace yave {


WaitToken::WaitToken(const TimelineFence& fence) : _fence(fence) {
}

void WaitToken::wait() {
    wait_for_fence(_fence);
}


CmdQueueBase::CmdQueueBase(u32 family_index, VkQueue queue, bool is_async) : _queue(queue), _family_index(family_index), _is_async_queue(is_async) {
}

CmdQueueBase::~CmdQueueBase() {
    wait();
}

u32 CmdQueueBase::family_index() const {
    return _family_index;
}

void CmdQueueBase::wait() const {
    _queue.locked([](auto&& queue) {
        vk_check(vkQueueWaitIdle(queue));
    });
}

WaitToken CmdQueueBase::submit(CmdBufferRecorderBase&& recorder, VkSemaphore wait, VkSemaphore signal, VkFence fence, bool async_start) const {
    y_profile();

    const VkCommandBuffer cmd_buffer = recorder.vk_cmd_buffer();
    vk_check(vkEndCommandBuffer(cmd_buffer));

    TimelineFence timeline_fence;

    _queue.locked([&](auto&& queue) {
        // This needs to be inside the lock
        timeline_fence = create_timeline_fence();
        const u64 prev_value = timeline_fence._value - 1;

        recorder._data->_timeline_fence = timeline_fence;

        const VkSemaphore timeline_semaphore = vk_timeline_semaphore();

        u32 wait_count = 0;
        std::array<VkSemaphore, 2> wait_semaphores;
        std::array<u64, 2> wait_values;

        Y_TODO(We need to ensure that the main timeline semaphore is always incremented in order)
        Y_TODO(This means that we NEED cmd buffers to end, and signal, in order, so we have to wait on the previous one here)
        unused(async_start);
        /*if(!_is_async_queue && !async_start)*/ {
            wait_values[wait_count] = prev_value;
            wait_semaphores[wait_count] = timeline_semaphore;
            ++wait_count;
        }
        if(wait) {
            wait_values[wait_count] = 0;
            wait_semaphores[wait_count] = wait;
             ++wait_count;
        }

        const std::array<u64, 2> signal_values = {timeline_fence._value, 0};
        const std::array<VkSemaphore, 2> signal_semaphores = {timeline_semaphore, signal};
        const u32 signal_count = signal_semaphores[1] ? 2 : 1;

        const std::array<VkPipelineStageFlags, 2> pipe_stage_flags = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};

        VkTimelineSemaphoreSubmitInfo timeline_info = vk_struct();
        {
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
            submit_info.pWaitDstStageMask = pipe_stage_flags.data();
            submit_info.waitSemaphoreCount = wait_count;
            submit_info.pWaitSemaphores = wait_semaphores.data();
            submit_info.signalSemaphoreCount = signal_count;
            submit_info.pSignalSemaphores = signal_semaphores.data();
        }

        y_profile_zone("submit");
        vk_check(vkQueueSubmit(queue, 1, &submit_info, fence));
    });

    lifetime_manager().register_for_polling(std::exchange(recorder._data, nullptr));

    return WaitToken(timeline_fence);
}


CmdQueue::CmdQueue(u32 family_index, VkQueue queue) : CmdQueueBase(family_index, queue, false) {
}

WaitToken CmdQueue::submit(TransferCmdBufferRecorder&& recorder) const {
    return CmdQueueBase::submit(std::move(recorder));
}

WaitToken CmdQueue::submit_async_start(TransferCmdBufferRecorder&& recorder) const {
    return CmdQueueBase::submit(std::move(recorder), {}, {}, {}, true);
}

WaitToken CmdQueue::submit(CmdBufferRecorder&& recorder) const {
    return CmdQueueBase::submit(std::move(recorder));
}

}

