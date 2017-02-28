/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

#include "CmdBufferBase.h"
#include "CmdBufferPoolData.h"
#include <yave/device/Device.h>

namespace yave {

CmdBufferBase::CmdBufferBase(const core::Arc<CmdBufferPoolData>& pool) : _pool(pool), _data(_pool->alloc()) {
}

CmdBufferBase::CmdBufferBase(CmdBufferBase&& other) : CmdBufferBase() {
	swap(other);
}

CmdBufferBase::~CmdBufferBase() {
	if(_pool) {
		_pool->release(std::move(_data));
	}
}

void CmdBufferBase::swap(CmdBufferBase& other) {
	std::swap(_pool, other._pool);
	std::swap(_data, other._data);
}

void CmdBufferBase::submit(vk::Queue queue) {
	device()->vk_device().resetFences(_data.fence);
	queue.submit(vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&_data.cmd_buffer), _data.fence
		);
}

DevicePtr CmdBufferBase::device() const {
	return _pool->device();
}

const vk::CommandBuffer CmdBufferBase::vk_cmd_buffer() const {
	return _data.cmd_buffer;
}

vk::Fence CmdBufferBase::vk_fence() const {
	return _data.fence;
}

}
