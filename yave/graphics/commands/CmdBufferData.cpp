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

#include "CmdBufferData.h"

#include <yave/graphics/commands/CmdBufferPool.h>
#include <yave/graphics/device/LifetimeManager.h>
#include <yave/graphics/graphics.h>

#ifdef Y_DEBUG
#define YAVE_CMD_CHECK_LOCK()                                                           \
    y_debug_assert(_lock.exchange(true, std::memory_order_acquire) == false);           \
    y_defer(y_debug_assert(_lock.exchange(false, std::memory_order_acquire) == true))
#else
#define YAVE_CMD_CHECK_LOCK()   do {} while(false)
#endif

namespace yave {

CmdBufferData::CmdBufferData(VkCommandBuffer buf, VkFence fen, CmdBufferPool* p) :
        _cmd_buffer(buf), _fence(fen), _pool(p), _resource_fence(lifetime_manager().create_fence()) {
}

CmdBufferData::~CmdBufferData() {
    YAVE_CMD_CHECK_LOCK();
    y_debug_assert(!_pool || vkGetFenceStatus(vk_device(), _fence) == VK_SUCCESS);
}

bool CmdBufferData::is_null() const {
    return !_cmd_buffer;
}

bool CmdBufferData::is_signaled() const {
    return _signaled;
}

CmdBufferPool* CmdBufferData::pool() const {
    return _pool;
}

VkCommandBuffer CmdBufferData::vk_cmd_buffer() const {
    return _cmd_buffer;
}

VkFence CmdBufferData::vk_fence() const {
    return _fence;
}

ResourceFence CmdBufferData::resource_fence() const {
    return _resource_fence;
}

void CmdBufferData::wait() {
    y_profile();
    if(!is_signaled()) {
        vk_check(vkWaitForFences(vk_device(), 1, &_fence, true, u64(-1)));
        set_signaled();
    }
}

bool CmdBufferData::poll_and_signal() {
    if(is_signaled()) {
        return true;
    }
    if(vkGetFenceStatus(vk_device(), _fence) == VK_SUCCESS) {
        set_signaled();
        return true;
    }
    return false;
}

void CmdBufferData::begin() {
    YAVE_CMD_CHECK_LOCK();
    y_profile();


    y_debug_assert(is_signaled());
    y_debug_assert(_keep_alive.is_empty());

    vk_check(vkResetFences(vk_device(), 1, &_fence));
    vk_check(vkResetCommandBuffer(_cmd_buffer, 0));

    _resource_fence = lifetime_manager().create_fence();
    _signaled = false;
}

void CmdBufferData::recycle_resources() {
    YAVE_CMD_CHECK_LOCK();
    y_profile();

    y_debug_assert(is_signaled());
    _keep_alive.clear();
}

void CmdBufferData::set_signaled() {
    _signaled.exchange(true, std::memory_order_acquire);
}

}

#undef YAVE_CMD_CHECK_LOCK

