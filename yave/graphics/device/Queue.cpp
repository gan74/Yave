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

#include "Queue.h"

namespace yave {

Queue::Queue(u32 family_index, VkQueue queue) :
        _queue(queue),
        _family_index(family_index),
        _lock(std::make_unique<std::mutex>()){
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

void Queue::submit(CmdBufferRecorder& rec) const {
    y_profile();

    y_debug_assert(vkGetFenceStatus(vk_device(), rec.vk_fence()) == VK_NOT_READY);

    const VkCommandBuffer cmd = rec.vk_cmd_buffer();

    {
        const auto lock = y_profile_unique_lock(*_lock);

        VkSubmitInfo submit_info = vk_struct();
        {
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmd;
        }

        vk_check(vkQueueSubmit(_queue, 1, &submit_info, rec.vk_fence()));
    }
}

}

