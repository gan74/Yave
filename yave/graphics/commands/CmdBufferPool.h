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
#ifndef YAVE_GRAPHICS_COMMANDS_CMDBUFFERPOOL_H
#define YAVE_GRAPHICS_COMMANDS_CMDBUFFERPOOL_H

#include <yave/yave.h>
#include <yave/graphics/graphics.h>
#include <yave/graphics/commands/CmdBufferData.h>

#include <yave/utils/traits.h>

#include <y/concurrent/Mutexed.h>
#include <thread>

namespace yave {

class CmdBufferPool : NonMovable {

    public:
        CmdBufferPool();
        ~CmdBufferPool();

        VkCommandPool vk_pool() const;

        CmdBufferRecorder create_cmd_buffer();
        ComputeCmdBufferRecorder create_compute_cmd_buffer();
        TransferCmdBufferRecorder create_transfer_cmd_buffer();

    private:
        friend class LifetimeManager;

        void release(CmdBufferData* data);

        CmdBufferData* alloc();

        void join_all();

    private:
        CmdBufferData* create_data();

        core::Vector<std::unique_ptr<CmdBufferData>> _cmd_buffers;
        VkHandle<VkCommandPool> _pool; // Synced through _cmd_buffers

        concurrent::Mutexed<core::Vector<CmdBufferData*>> _released;

#ifdef Y_DEBUG
        std::thread::id _thread_id;
#endif
};

}

#endif // YAVE_GRAPHICS_COMMANDS_CMDBUFFERPOOL_H

