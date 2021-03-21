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

#include "CmdBufferPool.h"
#include "CmdBuffer.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/Queue.h>
#include <yave/graphics/device/LifetimeManager.h>

#include <y/core/Chrono.h>
#include <y/concurrent/concurrent.h>

namespace yave {

static VkCommandPool create_pool() {
    VkCommandPoolCreateInfo create_info = vk_struct();
    {
        create_info.queueFamilyIndex = graphic_queue().family_index();
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }

    VkCommandPool pool = {};
    vk_check(vkCreateCommandPool(vk_device(), &create_info, vk_allocation_callbacks(), &pool));
    return pool;
}


CmdBufferPool::CmdBufferPool() :
        _pool(create_pool()),
        _thread_id(concurrent::thread_id()) {
}

CmdBufferPool::~CmdBufferPool() {
    join_all();

    y_debug_assert(_cmd_buffers.size() == _released.size());

    for(const VkFence fence : _fences) {
        device_destroy(fence);
    }
    device_destroy(_pool);
}

VkCommandPool CmdBufferPool::vk_pool() const {
    return _pool;
}

void CmdBufferPool::join_all() {
    y_profile();

    const auto lock = y_profile_unique_lock(_pool_lock);
    for(auto& data : _cmd_buffers) {
        data->wait();
    }
}

void CmdBufferPool::release(CmdBufferData* data) {
    y_profile();

    y_debug_assert(data->pool() == this);
    y_debug_assert(data->is_signaled());

    data->recycle_resources();

    {
        const auto lock = y_profile_unique_lock(_release_lock);
        _released << data;
    }
}

CmdBufferData* CmdBufferPool::alloc() {
    y_profile();
    y_debug_assert(concurrent::thread_id() == _thread_id);

    CmdBufferData* ready = nullptr;

    {
        const auto lock = y_profile_unique_lock(_release_lock);

        if(!_released.is_empty()) {
            ready = _released.pop();
        }
    }

    if(ready) {
        ready->begin();
        return ready;
    }

    return create_data();
}

CmdBufferData* CmdBufferPool::create_data() {
    const auto lock = y_profile_unique_lock(_pool_lock);

    const VkFenceCreateInfo fence_create_info = vk_struct();
    VkCommandBufferAllocateInfo allocate_info = vk_struct();
    {
        allocate_info.commandBufferCount = 1;
        allocate_info.commandPool = _pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }

    VkCommandBuffer buffer = {};
    VkFence fence = {};
    vk_check(vkAllocateCommandBuffers(vk_device(), &allocate_info, &buffer));
    vk_check(vkCreateFence(vk_device(), &fence_create_info, vk_allocation_callbacks(), &fence));

    _fences << fence;
    return _cmd_buffers.emplace_back(std::make_unique<CmdBufferData>(buffer, fence, this)).get();
}


CmdBuffer CmdBufferPool::create_buffer() {
    return CmdBuffer(alloc());
}

}

