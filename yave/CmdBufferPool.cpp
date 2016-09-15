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

#include "CmdBufferPool.h"
#include "Device.h"

namespace yave {

vk::CommandPool create_pool(DevicePtr dptr) {
	return dptr->get_vk_device().createCommandPool(vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(dptr->get_queue_family_index(QueueFamily::Graphics))
			.setFlags(vk::CommandPoolCreateFlagBits::eTransient)
		);
}

CmdBufferPool::CmdBufferPool(DevicePtr dptr) : DeviceLinked(dptr), _pool(create_pool(dptr)) {
}

CmdBufferPool::~CmdBufferPool() {
	for(const auto& buffer : _cmd_buffers) {
		/*if(buffer.ref_count() != 1) {
			fatal("CmdBufferState was not deleted before its pool");
		}*/
		buffer->wait();
	}
	_cmd_buffers.clear();
	//std::cout << "pool (" << _pool << ") deleted\n";
	destroy(_pool);
}

CmdBufferPool::CmdBufferPool(CmdBufferPool&& other) {
	swap(other);
}

CmdBufferPool &CmdBufferPool::operator=(CmdBufferPool&& other) {
	swap(other);
	return *this;
}

void CmdBufferPool::swap(CmdBufferPool& other) {
	DeviceLinked::swap(other);
	std::swap(_pool, other._pool);
	std::swap(_cmd_buffers, other._cmd_buffers);
}

core::Rc<CmdBufferState> CmdBufferPool::get_buffer() {
	if(_cmd_buffers.is_empty()) {
		return core::Rc<CmdBufferState>(new CmdBufferState(get_device(), _pool));
	}
	auto buffer = _cmd_buffers.last();
	_cmd_buffers.pop();
	return buffer;
}



}
