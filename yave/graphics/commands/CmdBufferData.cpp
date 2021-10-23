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

namespace yave {


bool ResourceFence::operator==(const ResourceFence& other) const {
    return _value == other._value;
}

bool ResourceFence::operator!=(const ResourceFence& other) const {
    return _value != other._value;
}


bool ResourceFence::operator<(const ResourceFence& other) const {
    return _value < other._value;
}

bool ResourceFence::operator<=(const ResourceFence& other) const {
    return _value <= other._value;
}

ResourceFence::ResourceFence(u64 v) : _value(v) {
}

bool QueueFence::is_null() const {
    return !_semaphore;
}

VkSemaphoreWaitInfo QueueFence::vk_wait_info() const {
    y_debug_assert(!is_null());

    VkSemaphoreWaitInfo wait_info = vk_struct();
    {
        wait_info.pSemaphores = &_semaphore;
        wait_info.pValues = &_value;
        wait_info.semaphoreCount = 1;
    }
    return wait_info;
}

bool QueueFence::operator==(const QueueFence& other) const {
    return _value == other._value;
}

bool QueueFence::operator!=(const QueueFence& other) const {
    return _value != other._value;
}



CmdBufferData::CmdBufferData(VkCommandBuffer buf, CmdBufferPool* p) :
        _cmd_buffer(buf), _pool(p), _resource_fence(lifetime_manager().create_fence()) {
}

CmdBufferData::~CmdBufferData() {
    y_debug_assert(!_pool || poll());
}

bool CmdBufferData::is_null() const {
    return !_cmd_buffer;
}

CmdBufferPool* CmdBufferData::pool() const {
    return _pool;
}

VkCommandBuffer CmdBufferData::vk_cmd_buffer() const {
    return _cmd_buffer;
}

ResourceFence CmdBufferData::resource_fence() const {
    return _resource_fence;
}

QueueFence CmdBufferData::queue_fence() const {
    return _queue_fence;
}

void CmdBufferData::wait() {
    y_profile();
    if(!_signaled) {
        VkSemaphoreWaitInfo wait_info = _queue_fence.vk_wait_info();
        vk_check(vkWaitSemaphores(vk_device(), &wait_info, u64(-1)));
        set_signaled();
    }
}

bool CmdBufferData::poll() {
    if(_signaled) {
        return true;
    }

    VkSemaphoreWaitInfo wait_info = _queue_fence.vk_wait_info();
    if(vkWaitSemaphores(vk_device(), &wait_info, u64(0)) == VK_SUCCESS) {
        set_signaled();
        return true;
    }
    return false;
}

void CmdBufferData::begin() {
    y_profile();

    y_debug_assert(_signaled);
    y_debug_assert(_keep_alive.is_empty());

    vk_check(vkResetCommandBuffer(_cmd_buffer, 0));

    _queue_fence = {};
    _resource_fence = lifetime_manager().create_fence();
    _signaled = false;
}

void CmdBufferData::recycle_resources() {
    y_profile();

    y_debug_assert(_signaled);

    _keep_alive.clear();
}

void CmdBufferData::set_signaled() {
    _signaled.exchange(true, std::memory_order_acquire);
}

}


