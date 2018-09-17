/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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

#include <mutex>

namespace yave {
#warning command buffers might need to be synchronized at the pool level

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

CmdBufferPoolBase::CmdBufferPoolBase(DevicePtr dptr, CmdBufferUsage preferred) :
		DeviceLinked(dptr),
		_pool(create_pool(dptr, preferred)),
		_usage(preferred) {
}

CmdBufferPoolBase::~CmdBufferPoolBase() {
	if(device()) {
		if(_fences.size() != _cmd_buffers.size()) {
			y_fatal("CmdBuffers are still in use.");
		}
		join_all();
		_cmd_buffers.clear();
		destroy(_pool);
	}
}

vk::CommandPool CmdBufferPoolBase::vk_pool() const {
	return _pool;
}

void CmdBufferPoolBase::join_all() {
	if(_fences.is_empty() || _usage == CmdBufferUsage::Secondary) {
		return;
	}

	if(device()->vk_device().waitForFences(_fences.size(), _fences.data(), true, u64(-1)) != vk::Result::eSuccess) {
		y_fatal("Unable to join fences.");
	}
}

void CmdBufferPoolBase::release(CmdBufferData&& data) {
	if(data.pool() != this) {
		y_fatal("CmdBufferData was not returned to its original pool.");
	}
	std::unique_lock lock(_lock);
	_cmd_buffers.push_back(std::move(data));
}

std::shared_ptr<CmdBufferDataProxy> CmdBufferPoolBase::alloc() {
	std::unique_lock lock(_lock);
	for(auto it = _cmd_buffers.begin(); it != _cmd_buffers.end(); ++it) {
		if(it->try_reset()) {
			auto ptr = std::make_shared<CmdBufferDataProxy>(std::move(*it));
			_cmd_buffers.erase(it);
			return ptr;
		}
	}
	return std::make_shared<CmdBufferDataProxy>(create_data());
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
			: device()->vk_device().createFence(vk::FenceCreateInfo());

	_fences << fence;
	//log_msg("new command buffer created (" + core::str(uenum(_usage)) + ") " + _cmd_buffers.size() + " waiting");
	return CmdBufferData(buffer, fence, this);
}

}
