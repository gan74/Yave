/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include <y/utils/memory.h>

namespace yave {

CmdBufferData::CmdBufferData(VkCommandBuffer buffer, CmdBufferPool* pool, VkCommandBufferLevel level) :
        _cmd_buffer(buffer),
        _pool(pool),
        _resource_fence(lifetime_manager().create_fence()),
        _level(level) {
}

CmdBufferData::~CmdBufferData() {
    y_debug_assert(!_pool || is_ready());
    destroy_graphic_resource(std::move(_semaphore));
}

void CmdBufferData::push_secondary(CmdBufferData* data) {
    y_debug_assert(!is_secondary());
    _secondaries << data;
}

CmdBufferData::InlineUniformSubBuffer CmdBufferData::alloc_inline_buffer(usize size) {
    y_profile();

    size = align_up_to(size, usize(InlineUniformSubBuffer::byte_alignment()));

    if(_inline_buffers.is_empty() || _inline_buffers.last()->byte_size() - _inline_buffer_offset < size) {
        _inline_buffers.emplace_back(_pool->alloc_inline_uniform_buffer());
        _inline_buffer_offset = 0;
    }

    InlineUniformBuffer& buffer = *_inline_buffers.last();
    y_debug_assert(size <= buffer.byte_size());

    InlineUniformSubBuffer sub_buffer(buffer, size, _inline_buffer_offset);
    _inline_buffer_offset += size;

    return sub_buffer;
}

bool CmdBufferData::is_null() const {
    return !_cmd_buffer;
}

bool CmdBufferData::is_secondary() const {
    return _level == VK_COMMAND_BUFFER_LEVEL_SECONDARY;
}

CmdBufferPool* CmdBufferData::pool() const {
    return _pool;
}

CmdQueue* CmdBufferData::queue() const {
    return _pool->queue();
}

VkCommandBuffer CmdBufferData::vk_cmd_buffer() const {
    return _cmd_buffer;
}

ResourceFence CmdBufferData::resource_fence() const {
    return _resource_fence;
}

TimelineFence CmdBufferData::timeline_fence() const {
    return _timeline_fence;
}

const CmdBufferFence& CmdBufferData::create_fence() {
    if(!_fence._ptr) {
        _fence._ptr  = std::make_shared<std::atomic<bool>>(false);
    }
    return _fence;
}

bool CmdBufferData::is_ready() const {
    return _timeline_fence.is_valid() && _timeline_fence.is_ready();
}

void CmdBufferData::reset() {
    y_profile();

    vk_check(vkResetCommandBuffer(_cmd_buffer, 0));

    _resource_fence = lifetime_manager().create_fence();
    _timeline_fence = {};

    y_debug_assert(!_fence._ptr);
}

}


