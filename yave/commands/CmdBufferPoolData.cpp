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

#include "CmdBufferPoolData.h"
#include <yave/device/Device.h>

#include <y/core/Chrono.h>

namespace yave {

/*static vk::CommandBufferUsageFlagBits cmd_usage(CmdBufferUsage u) {
	return vk::CommandBufferUsageFlagBits(uenum(u) & ~uenum(CmdBufferUsage::Secondary));
}*/

static vk::CommandBufferLevel cmd_level(CmdBufferUsage u) {
	return u == CmdBufferUsage::Secondary ? vk::CommandBufferLevel::eSecondary : vk::CommandBufferLevel::ePrimary;
}

static vk::CommandPoolCreateFlagBits cmd_create_flags(CmdBufferUsage u) {
	return u == CmdBufferUsage::Disposable ? vk::CommandPoolCreateFlagBits::eTransient : vk::CommandPoolCreateFlagBits();
}



static vk::CommandPool create_pool(DevicePtr dptr, CmdBufferUsage usage) {
	return dptr->vk_device().createCommandPool(vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(dptr->queue_family(QueueFamily::Graphics).index())
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer | cmd_create_flags(usage))
		);
}

static CmdBufferData alloc_data(DevicePtr dptr, vk::CommandPool pool, CmdBufferUsage usage) {
	auto buffer = dptr->vk_device().allocateCommandBuffers(vk::CommandBufferAllocateInfo()
			.setCommandBufferCount(1)
			.setCommandPool(pool)
			.setLevel(cmd_level(usage))
		).back();
	auto fence = dptr->vk_device().createFence(vk::FenceCreateInfo()
			.setFlags(vk::FenceCreateFlagBits::eSignaled)
		);

	return CmdBufferData{buffer, fence};
}

static void wait(DevicePtr dptr, const CmdBufferData &data) {
	core::Chrono c;
	dptr->vk_device().waitForFences(data.fence, true, u64(-1));
	if(c.elapsed().to_nanos()) {
		log_msg(core::String() + "waited " + c.elapsed().to_micros() + "us for command buffer!", LogType::Warning);
	}
}

static void reset(const CmdBufferData &data) {
	data.cmd_buffer.reset(vk::CommandBufferResetFlags());
}

static const CmdBufferData& force_reset(DevicePtr dptr, const CmdBufferData &data) {
	wait(dptr, data);
	reset(data);
	return data;
}

static bool try_reset(DevicePtr dptr, const CmdBufferData& data) {
	if(dptr->vk_device().waitForFences(data.fence, true, 0) != vk::Result::eSuccess) {
		return false;
	}
	reset(data);
	return true;
}


CmdBufferPoolData::CmdBufferPoolData(DevicePtr dptr, CmdBufferUsage preferred) : DeviceLinked(dptr), _pool(create_pool(dptr, preferred)), _usage(preferred) {
}

CmdBufferPoolData::~CmdBufferPoolData() {
	join_fences();
	for(auto buffer : _cmd_buffers) {
		device()->vk_device().freeCommandBuffers(_pool, buffer.cmd_buffer);
		destroy(buffer.fence);
	}
	destroy(_pool);
}

void CmdBufferPoolData::join_fences() {
	auto fences = core::vector_with_capacity<vk::Fence>(_cmd_buffers.size());
	std::transform(_cmd_buffers.begin(), _cmd_buffers.end(), std::back_inserter(fences), [](auto& buffer) { return buffer.fence; });
	if(device()->vk_device().waitForFences(fences.size(), fences.data(), true, u64(-1)) != vk::Result::eSuccess) {
		fatal("Unable to join fences.");
	}
}

void CmdBufferPoolData::release(CmdBufferData&& data) {
	_cmd_buffers.push_back(std::move(data));
}

CmdBufferData CmdBufferPoolData::alloc() {
	if(_cmd_buffers.is_empty()) {
		return alloc_data(device(), _pool, _usage);
	}
	for(auto& buffer : _cmd_buffers) {
		if(try_reset(device(), buffer)) {
			std::swap(buffer, _cmd_buffers.last());
			return _cmd_buffers.pop();
		}
	}
	return force_reset(device(), _cmd_buffers.pop());
}

}
