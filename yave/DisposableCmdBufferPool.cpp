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

#include "DisposableCmdBufferPool.h"

namespace yave {

vk::CommandPool create_pool(DevicePtr dpr, Device::QueueFamilies family = Device::Graphics) {
	return dpr->get_vk_device().createCommandPool(vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(dpr->get_queue_family_index(family))
			.setFlags(vk::CommandPoolCreateFlagBits::eTransient)
		);
}

DisposableCmdBufferPool::DisposableCmdBufferPool(DevicePtr dptr) : DeviceLinked(dptr), _pool(create_pool(dptr)) {
}

DisposableCmdBufferPool::~DisposableCmdBufferPool() {
	destroy(_pool);
}

DisposableCmdBufferPool::Recorder DisposableCmdBufferPool::allocate() {
	auto cmd_buffer = get_device()->get_vk_device().allocateCommandBuffers(vk::CommandBufferAllocateInfo()
			.setCommandBufferCount(1)
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandPool(_pool)
		).back();

	cmd_buffer.begin(vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		);

	return Recorder(cmd_buffer/*, _pool*/);
}

}
