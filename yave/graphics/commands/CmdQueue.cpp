/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

CmdQueue::CmdQueue(u32 family_index, VkQueue queue) : _family_index(family_index), _queue(queue) {
}

CmdQueue::~CmdQueue() {
    wait();
}

u32 CmdQueue::family_index() const {
    return _family_index;
}

VkQueue CmdQueue::vk_queue() const {
    return _queue;
}

void CmdQueue::wait() const {
    const auto lock = y_profile_unique_lock(_lock);
    vk_check(vkQueueWaitIdle(_queue));
}

WaitToken CmdQueue::submit(CmdBufferRecorder&& recorder, VkSemaphore wait, VkSemaphore signal) const {
    y_profile();

    const VkCommandBuffer cmd_buffer = recorder.vk_cmd_buffer();
    vk_check(vkEndCommandBuffer(cmd_buffer));

    TimelineFence fence;

    {
        const auto lock = y_profile_unique_lock(_lock);

        // This needs to be inside the lock
        fence = create_timeline_fence();
        const u64 prev_value = fence._value - 1;

        recorder._data->_timeline_fence = fence;

        const VkSemaphore timeline_semaphore = vk_timeline_semaphore();

        Y_TODO(We do not need to wait on the timeline_semaphore if using SyncPolicy::Wait)
        const std::array<VkSemaphore, 2> wait_semaphores = {timeline_semaphore, wait};
        const std::array<VkSemaphore, 2> signal_semaphores = {timeline_semaphore, signal};

        const std::array<u64, 2> wait_values = {prev_value, 0};
        const std::array<u64, 2> signal_values = {fence._value, 0};

        const std::array<VkPipelineStageFlags, 2> pipe_stage_flags = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
        const u32 wait_count = wait_semaphores[1] ? 2 : 1;
        const u32 signal_count = signal_semaphores[1] ? 2 : 1;

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

        vk_check(vkQueueSubmit(_queue, 1, &submit_info, {}));
    }

    lifetime_manager().register_for_polling(std::exchange(recorder._data, nullptr));

    return WaitToken(fence);
}

}

