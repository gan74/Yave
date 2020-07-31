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

#include <yave/device/DeviceUtils.h>

#include "Queue.h"

namespace yave {

Queue::Queue(DevicePtr dptr, VkQueue queue) :
        DeviceLinked(dptr),
        _queue(queue),
        _lock(std::make_unique<std::mutex>()){
}

Queue::~Queue() {
    if(device()) {
        wait();
    }
}


VkQueue Queue::vk_queue() const {
    return _queue;
}

void Queue::wait() const {
    const auto lock = y_profile_unique_lock(*_lock);
    vk_check(vkQueueWaitIdle(_queue));
}

Semaphore Queue::submit_sem(RecordedCmdBuffer&& cmd) const {
    const Semaphore sync(device());
    cmd._proxy->data()._signal = sync;
    submit_base(cmd);
    return sync;
}

void Queue::submit_base(CmdBufferBase& base) const {
    const auto lock = y_profile_unique_lock(*_lock);

    auto cmd = base.vk_cmd_buffer();

    const auto& wait = base._proxy->data()._waits;
    auto wait_semaphores = core::vector_with_capacity<VkSemaphore>(wait.size());
    std::transform(wait.begin(), wait.end(), std::back_inserter(wait_semaphores), [](const auto& s) { return s.vk_semaphore(); });
    const core::Vector<VkPipelineStageFlags> stages(wait.size(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    const Semaphore& signal = base._proxy->data()._signal;
    const VkSemaphore sig_semaphore = signal.device() ? signal.vk_semaphore() : VkSemaphore{};

    VkSubmitInfo submit_info = vk_struct();
    {
        submit_info.signalSemaphoreCount = signal.device() ? 1 : 0;
        submit_info.pSignalSemaphores = signal.device() ? &sig_semaphore : nullptr;
        submit_info.waitSemaphoreCount = wait_semaphores.size();
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = stages.data();
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd;
    }

    vk_check(vkQueueSubmit(_queue, 1, &submit_info, base.vk_fence()));
}

}


