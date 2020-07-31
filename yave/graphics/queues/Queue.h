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
#ifndef YAVE_GRAPHICS_QUEUES_QUEUE_H
#define YAVE_GRAPHICS_QUEUES_QUEUE_H

#include <yave/graphics/vk/vk.h>

#include "Semaphore.h"
#include "submit.h"

#include <mutex>

namespace yave {

class Queue : NonCopyable, public DeviceLinked {

    public:
        Queue() = default;
        Queue(Queue&&) = default;
        Queue& operator=(Queue&&) = default;

        ~Queue();

        VkQueue vk_queue() const;

        void wait() const;

        Semaphore submit_sem(RecordedCmdBuffer&& cmd) const;

        template<typename SyncPolicy>
        void submit(RecordedCmdBuffer&& cmd, const SyncPolicy& policy = SyncPolicy()) const {
            y_profile();
            submit_base(cmd);
            policy(cmd);
        }

        std::mutex& lock() const {
            return *_lock;
        }

    private:
        friend class QueueFamily;

        Queue(DevicePtr dptr, VkQueue queue);

        void submit_base(CmdBuffer& base) const;

        SwapMove<VkQueue> _queue;
        std::unique_ptr<std::mutex> _lock;
};

}

#endif // YAVE_GRAPHICS_QUEUES_QUEUE_H

