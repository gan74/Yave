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

#include <yave/graphics/graphics.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>

#include "Device.h"
#include "Queue.h"

namespace yave {

Queue::Queue(u32 family_index, VkQueue queue) :
        _queue(queue),
        _family_index(family_index),
        _lock(std::make_unique<std::mutex>()) {
}

Queue::~Queue() {
    if(_queue && _lock) {
        wait();
    }
}


VkQueue Queue::vk_queue() const {
    return _queue;
}

u32 Queue::family_index() const {
    return _family_index;
}

void Queue::wait() const {
    const auto lock = y_profile_unique_lock(*_lock);
    vk_check(vkQueueWaitIdle(_queue));
}

void Queue::end_and_submit(CmdBufferRecorder& recorder, VkSemaphore wait, VkSemaphore signal) const {
    y_profile();

    const VkCommandBuffer cmd_buffer = recorder.vk_cmd_buffer();
    vk_check(vkEndCommandBuffer(cmd_buffer));

    {
        const auto lock = y_profile_unique_lock(*_lock);

        // This needs to be inside the lock
        const QueueFence fence = main_device()->create_fence();
        const u64 prev_value = fence._value - 1;

#ifdef YAVE_CHECK_QUEUE_SYNC
        y_always_assert(_last_fence < fence.value(), "Deadlock!");
        _last_fence = fence.value();
#endif

        recorder._data->_queue_fence = fence;

        const VkSemaphore timeline_semaphore = main_device()->vk_timeline_semaphore();

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
}

}

