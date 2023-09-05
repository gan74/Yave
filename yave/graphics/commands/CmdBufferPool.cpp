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

#include "CmdBufferPool.h"
#include "CmdBufferRecorder.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/device/LifetimeManager.h>

#include <y/core/Chrono.h>
#include <y/concurrent/concurrent.h>

namespace yave {

static VkHandle<VkCommandPool> create_pool() {
    VkCommandPoolCreateInfo create_info = vk_struct();
    {
        create_info.queueFamilyIndex = command_queue().family_index();
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    }

    VkHandle<VkCommandPool> pool;
    vk_check(vkCreateCommandPool(vk_device(), &create_info, vk_allocation_callbacks(), pool.get_ptr_for_init()));
    return pool;
}


CmdBufferPool::CmdBufferPool() : _pool(create_pool()) {
#ifdef Y_DEBUG
    _thread_id = std::this_thread::get_id();
#endif
}

CmdBufferPool::~CmdBufferPool() {
    y_debug_assert(_thread_id == std::this_thread::get_id());

    join_all();

    y_debug_assert( _cmd_buffers.size() == _released.locked([&](auto&& released) { return released.size(); }));

    destroy_graphic_resource(std::move(_pool));
}

VkCommandPool CmdBufferPool::vk_pool() const {
    return _pool;
}

void CmdBufferPool::join_all() {
    y_profile();

    y_debug_assert(_thread_id == std::this_thread::get_id());

    for(;;) {
        for(auto& data : _cmd_buffers) {
            data->wait();
        }

        lifetime_manager().poll_cmd_buffers();

        if(_cmd_buffers.size() ==  _released.locked([&](auto&& released) { return released.size(); })) {
            break;
        }
    }
}

void CmdBufferPool::release(CmdBufferData* data) {
    y_profile();

    y_debug_assert(data->pool() == this);
    y_debug_assert(data->poll());

    _released.locked([&](auto&& released) { released << data; });
}

CmdBufferData* CmdBufferPool::alloc() {
    y_profile();

    y_debug_assert(_thread_id == std::this_thread::get_id());

    CmdBufferData* ready = nullptr;

    _released.locked([&](auto&& released) {
        if(!released.is_empty()) {
            ready = released.pop();
        }
    });

    if(ready) {
        ready->begin();
        return ready;
    }

    return create_data();
}

CmdBufferData* CmdBufferPool::create_data() {
    VkCommandBufferAllocateInfo allocate_info = vk_struct();
    {
        allocate_info.commandBufferCount = 1;
        allocate_info.commandPool = _pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }

    VkCommandBuffer buffer = {};
    vk_check(vkAllocateCommandBuffers(vk_device(), &allocate_info, &buffer));

    return _cmd_buffers.emplace_back(std::make_unique<CmdBufferData>(buffer, this)).get();
}

CmdBufferRecorder CmdBufferPool::create_cmd_buffer() {
    return CmdBufferRecorder(alloc());
}

ComputeCmdBufferRecorder CmdBufferPool::create_compute_cmd_buffer() {
    return ComputeCmdBufferRecorder(alloc());
}

TransferCmdBufferRecorder CmdBufferPool::create_transfer_cmd_buffer() {
    return TransferCmdBufferRecorder(alloc());
}



}

