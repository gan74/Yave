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
#ifndef YAVE_GRAPHICS_COMMANDS_CMDBUFFER_H
#define YAVE_GRAPHICS_COMMANDS_CMDBUFFER_H

#include <yave/graphics/commands/CmdBufferData.h>

namespace yave {

struct CmdBuffer : NonCopyable {
    public:
        CmdBuffer() = default;

        DevicePtr device() const;

        VkCommandBuffer vk_cmd_buffer() const;
        VkFence vk_fence() const;
        ResourceFence resource_fence() const;

        void wait() const;
        void wait_for(const Semaphore& sem);

        template<typename T>
        T wait_for(BoxSemaphore<T>&& t) {
            CmdBuffer::wait_for(static_cast<const Semaphore&>(t));
            return std::move(t._boxed);
        }

        template<typename T>
        void keep_alive(T&& t) {
            _proxy->data().keep_alive(y_fwd(t));
        }

    protected:
        CmdBuffer(std::unique_ptr<CmdBufferDataProxy>&& proxy);

        void release();

    private:
        friend class CmdBufferPool;
        friend class Queue;

        std::unique_ptr<CmdBufferDataProxy> _proxy;

};

}

#endif // YAVE_GRAPHICS_COMMANDS_CMDBUFFER_H

