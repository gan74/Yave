/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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
#include "CmdBufferPoolData.h"
#include <yave/Device.h>

namespace yave {

CmdBuffer::CmdBuffer(const core::Rc<CmdBufferPoolData>& pool) : _pool(pool), _data(_pool->alloc()) {
}

CmdBuffer::~CmdBuffer() {
	if(_pool) {
		_pool->release(std::move(_data));
	}
}

CmdBuffer::CmdBuffer(CmdBuffer&& other) {
	swap(other);
}

CmdBuffer& CmdBuffer::operator=(CmdBuffer&& other) {
	swap(other);
	return *this;
}

void CmdBuffer::swap(CmdBuffer& other) {
	std::swap(_pool, other._pool);
	std::swap(_data, other._data);
}

void CmdBuffer::submit(vk::Queue queue) {
	device()->vk_device().resetFences(_data.fence);
	queue.submit(vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&_data.cmd_buffer), _data.fence
		);
}

DevicePtr CmdBuffer::device() const {
	return _pool->device();//_pool ? _pool->device() : nullptr;
}

const vk::CommandBuffer CmdBuffer::vk_cmd_buffer() const {
	return _data.cmd_buffer;
}

vk::Fence CmdBuffer::vk_fence() const {
	return _data.fence;
}

}
