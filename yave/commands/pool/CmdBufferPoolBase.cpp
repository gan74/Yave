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

#include "CmdBufferPoolBase.h"
#include <yave/device/Device.h>

#include <y/core/Chrono.h>

namespace yave {

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


static void reset(CmdBufferData& data, CmdBufferUsage) {
	data.dependencies.clear();
	data.cmd_buffer.reset(vk::CommandBufferResetFlags());
}

static bool try_reset(CmdBufferData& data, CmdBufferUsage usage) {
	if(!data.wait_for_fences(0)) {
		return false;
	}
	reset(data, usage);
	return true;
}


CmdBufferPoolBase::CmdBufferPoolBase(DevicePtr dptr, CmdBufferUsage preferred) :
		DeviceLinked(dptr),
		_pool(create_pool(dptr, preferred)),
		_usage(preferred) {
}

CmdBufferPoolBase::~CmdBufferPoolBase() {
	join_fences();
	for(auto& buffer : _cmd_buffers) {
		device()->vk_device().freeCommandBuffers(_pool, buffer.cmd_buffer);
		destroy(buffer.fence);
	}
	destroy(_pool);
}

void CmdBufferPoolBase::swap(CmdBufferPoolBase& other) {
	DeviceLinked::swap(other);
	std::swap(_pool, other._pool);
	std::swap(_usage, other._usage);
	std::swap(_cmd_buffers, other._cmd_buffers);
}

void CmdBufferPoolBase::join_fences() {
	if(_cmd_buffers.is_empty()) {
		return;
	}

	auto fences = core::vector_with_capacity<vk::Fence>(_cmd_buffers.size());
	for(const auto&buffer : _cmd_buffers) {
		if(buffer.fence) {
			fences.push_back(buffer.fence);
		}
		fences.push_back(buffer.dependencies.begin(), buffer.dependencies.end());
	}

	if(device()->vk_device().waitForFences(fences.size(), fences.data(), true, u64(-1)) != vk::Result::eSuccess) {
		fatal("Unable to join fences.");
	}
}

void CmdBufferPoolBase::release(CmdBufferData&& data) {
	if(data.pool != this) {
		fatal("CmdBufferData was not returned to its original pool.");
	}
	_cmd_buffers.push_back(std::move(data));
}

CmdBufferData CmdBufferPoolBase::alloc() {
	for(auto& buffer : _cmd_buffers) {
		if(try_reset(buffer, _usage)) {
			std::swap(buffer, _cmd_buffers.last());
			return _cmd_buffers.pop();
		}
	}
	return create_data();
}

CmdBufferData CmdBufferPoolBase::create_data() {
	auto buffer = device()->vk_device().allocateCommandBuffers(vk::CommandBufferAllocateInfo()
			.setCommandBufferCount(1)
			.setCommandPool(_pool)
			.setLevel(cmd_level(_usage))
		).back();

	auto fence =
		(_usage == CmdBufferUsage::Secondary)
			? vk::Fence()
			: device()->vk_device().createFence(vk::FenceCreateInfo()
					.setFlags(vk::FenceCreateFlagBits::eSignaled)
				);

	return CmdBufferData(buffer, fence, this);
}

}
