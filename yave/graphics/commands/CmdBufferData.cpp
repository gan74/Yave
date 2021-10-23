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
#include <yave/graphics/device/Device.h>

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




u64 QueueFence::value() const {
    return _value;
}

bool QueueFence::operator==(const QueueFence& other) const {
    return _value == other._value;
}

bool QueueFence::operator!=(const QueueFence& other) const {
    return _value != other._value;
}

bool QueueFence::operator<(const QueueFence& other) const {
    return _value < other._value;
}

bool QueueFence::operator<=(const QueueFence& other) const {
    return _value <= other._value;
}

QueueFence::QueueFence(u64 v) : _value(v) {
}




CmdBufferData::CmdBufferData(VkCommandBuffer buf, CmdBufferPool* p) :
        _cmd_buffer(buf),
        _pool(p),
        _resource_fence(lifetime_manager().create_fence()) {
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
    main_device()->wait_for_fence(_queue_fence);
}

bool CmdBufferData::poll() {
    return main_device()->poll_fence(_queue_fence);
}

void CmdBufferData::begin() {
    y_profile();

    y_debug_assert(_keep_alive.is_empty());

    vk_check(vkResetCommandBuffer(_cmd_buffer, 0));

    _resource_fence = lifetime_manager().create_fence();
}

void CmdBufferData::recycle_resources() {
    y_profile();

    _keep_alive.clear();
}


}


