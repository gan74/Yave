/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#include "CmdBuffer.h"
#include "CmdBufferPoolData.h"
#include <yave/Device.h>

namespace yave {

CmdBuffer::CmdBuffer() {
}

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
	get_device()->get_vk_device().resetFences(_data.fence);
	queue.submit(vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&_data.cmd_buffer), _data.fence
		);
}

DevicePtr CmdBuffer::get_device() const {
	return _pool->get_device();//_pool ? _pool->get_device() : nullptr;
}

const vk::CommandBuffer CmdBuffer::get_vk_cmd_buffer() const {
	return _data.cmd_buffer;
}

vk::Fence CmdBuffer::get_vk_fence() const {
	return _data.fence;
}

}
