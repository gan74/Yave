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

#include "CmdBufferPoolData.h"
#include <yave/Device.h>


namespace yave {

vk::CommandPool create_pool(DevicePtr dptr) {
	return dptr->get_vk_device().createCommandPool(vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(dptr->get_queue_family_index(QueueFamily::Graphics))
			//.setFlags(vk::CommandPoolCreateFlagBits::eTransient)
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		);
}

CmdBufferData alloc_data(DevicePtr dptr, vk::CommandPool pool) {
	auto buffer = dptr->get_vk_device().allocateCommandBuffers(vk::CommandBufferAllocateInfo()
			.setCommandBufferCount(1)
			.setCommandPool(pool)
		).back();
	auto fence = dptr->get_vk_device().createFence(vk::FenceCreateInfo()
			.setFlags(vk::FenceCreateFlagBits::eSignaled)
		);

	return CmdBufferData{buffer, fence};
}

void wait(DevicePtr dptr, const CmdBufferData &data) {
	Chrono c;
	dptr->get_vk_device().waitForFences(data.fence, true, u64(-1));
	if(c.elapsed().to_nanos()) {
		log_msg(core::String() + "waited " + c.elapsed().to_micros() + "us for command buffer!", LogType::Warning);
	}
}


CmdBufferPoolData::CmdBufferPoolData(DevicePtr dptr) : DeviceLinked(dptr), _pool(create_pool(dptr)) {
}

CmdBufferPoolData::~CmdBufferPoolData() {
	for(auto buffer : _cmd_buffers) {
		get_device()->get_vk_device().freeCommandBuffers(_pool, buffer.cmd_buffer);
		destroy(buffer.fence);
	}
	destroy(_pool);
}

void CmdBufferPoolData::release(CmdBufferData&& data) {
	/*std::cout << "buffer " << data.cmd_buffer << " released\n";
	std::cout << "buffer pool size for" << _pool << " = " << _cmd_buffers.size() << "\n";*/

	_cmd_buffers.append(std::move(data));
}

CmdBufferData CmdBufferPoolData::reset(CmdBufferData data) {
	wait(get_device(), data);
	data.cmd_buffer.reset(vk::CommandBufferResetFlags());
	return data;
}

CmdBufferData CmdBufferPoolData::alloc() {
	if(_cmd_buffers.is_empty()) {
		return alloc_data(get_device(), _pool);
	}
	return reset(_cmd_buffers.pop());
}

}
