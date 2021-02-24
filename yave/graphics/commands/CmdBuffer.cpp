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

#include "CmdBuffer.h"

#include <yave/graphics/commands/CmdBufferPool.h>
#include <yave/graphics/device/LifetimeManager.h>
#include <yave/graphics/utils.h>

namespace yave {

CmdBuffer::CmdBuffer(std::unique_ptr<CmdBufferData> data) : _data(std::move(data))  {
}

CmdBuffer::CmdBuffer(CmdBuffer&& other) {
    swap(other);
}

CmdBuffer& CmdBuffer::operator=(CmdBuffer&& other) {
    swap(other);
    return *this;
}

CmdBuffer::~CmdBuffer() {
    release();
}

void CmdBuffer::release() {
    if(DevicePtr dptr = device()) {
        lifetime_manager(dptr).recycle(std::move(_data));
    }
}

void CmdBuffer::wait() const {
    const VkFence fence = vk_fence();
    vk_check(vkWaitForFences(vk_device(device()), 1, &fence, true, u64(-1)));
}

DevicePtr CmdBuffer::device() const {
    Y_TODO(cache that?)
    const auto pool = _data ? _data->pool() : nullptr;
    return pool ? pool->device() : nullptr;
}

VkCommandBuffer CmdBuffer::vk_cmd_buffer() const {
    y_debug_assert(_data);
    return _data->vk_cmd_buffer();
}

VkFence CmdBuffer::vk_fence() const {
    y_debug_assert(_data);
    return  _data->vk_fence();
}

ResourceFence CmdBuffer::resource_fence() const {
    y_debug_assert(_data);
    return _data->resource_fence();
}

void CmdBuffer::swap(CmdBuffer &other) {
    std::swap(_data, other._data);
}


}

