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
#ifndef YAVE_GRAPHICS_COMMANDS_CMDQUEUE_H
#define YAVE_GRAPHICS_COMMANDS_CMDQUEUE_H

#include "CmdBufferRecorder.h"

#include <y/concurrent/Mutexed.h>

#include <memory>
#include <variant>

namespace yave {

class WaitToken {
    public:
        void wait();

    private:
        friend class CmdQueue;

        WaitToken(const TimelineFence& fence);

        TimelineFence _fence;
};


class CmdQueue final : NonMovable {
    public:
        CmdQueue(u32 family_index, VkQueue queue);
        ~CmdQueue();

        u32 family_index() const;

        void wait();

        // Does not wait for the completion of previous commands before starting
        void submit_async_delayed_start(TransferCmdBufferRecorder&& recorder);
        void submit_async_delayed_start(ComputeCmdBufferRecorder&& recorder);

        WaitToken submit(TransferCmdBufferRecorder&& recorder);
        WaitToken submit(ComputeCmdBufferRecorder&& recorder);
        WaitToken submit(CmdBufferRecorder&& recorder);

    private:
        friend class Swapchain;


        WaitToken submit_internal(CmdBufferRecorderBase&& recorder, VkSemaphore wait = {}, VkSemaphore signal = {}, VkFence fence = {});

        concurrent::Mutexed<VkQueue> _queue = {};
        concurrent::Mutexed<core::Vector<CmdBufferData*>> _delayed_start;

        const u32 _family_index = u32(-1);
};

}

#endif // YAVE_GRAPHICS_COMMANDS_CMDQUEUE_H

