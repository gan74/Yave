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

#include "CmdBufferState.h"
#include "Device.h"

#include <iostream>

namespace yave {

vk::CommandBuffer create_buffer(DevicePtr dptr, vk::CommandPool pool) {
	return dptr->get_vk_device().allocateCommandBuffers(vk::CommandBufferAllocateInfo()
			.setCommandBufferCount(1)
			.setCommandPool(pool)
		).back();
}

vk::Fence create_fence(DevicePtr dptr) {
	return dptr->get_vk_device().createFence(vk::FenceCreateInfo()
			.setFlags(vk::FenceCreateFlagBits::eSignaled)
		);
}

CmdBufferState::CmdBufferState(DevicePtr dptr, vk::CommandPool pool) : DeviceLinked(dptr), _pool(pool), _cmd_buffer(create_buffer(dptr, pool)), _fence(create_fence(dptr)) {
}

CmdBufferState::~CmdBufferState() {
	wait();
	destroy(_fence);
	get_device()->get_vk_device().freeCommandBuffers(_pool, _cmd_buffer);
}

bool CmdBufferState::is_done() const {
	return get_device()->get_vk_device().waitForFences(_fence, true, 0) != vk::Result::eTimeout;
}

void CmdBufferState::wait() const {
	Chrono c;
	get_device()->get_vk_device().waitForFences(_fence, true, u64(-1));
	if(c.elapsed().to_nanos()) {
		std::cout << "waited " << c.elapsed().to_micros() << "us for command buffer!\n";
	}
}

void CmdBufferState::submit(vk::Queue queue) {
	get_device()->get_vk_device().resetFences(_fence);
	queue.submit(vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&_cmd_buffer), _fence
		);
}

const vk::CommandBuffer& CmdBufferState::get_vk_cmd_buffer() const {
	return _cmd_buffer;
}

vk::Fence CmdBufferState::get_vk_fence() const {
	return _fence;
}

}
