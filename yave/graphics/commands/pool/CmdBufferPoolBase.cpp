/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#include <y/concurrent/concurrent.h>

namespace yave {

static VkCommandBufferLevel cmd_level(CmdBufferUsage u) {
	unused(u);
	y_debug_assert(u == CmdBufferUsage::Disposable);
	return VK_COMMAND_BUFFER_LEVEL_PRIMARY;
}

static VkCommandPoolCreateFlagBits cmd_create_flags(CmdBufferUsage u) {
	return u == CmdBufferUsage::Disposable
		? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
		: VkCommandPoolCreateFlagBits(0);
}

static VkCommandPool create_pool(DevicePtr dptr, CmdBufferUsage usage) {
	VkCommandPoolCreateInfo create_info = vk_struct();
	{
		create_info.queueFamilyIndex = dptr->queue_family(QueueFamily::Graphics).index();
		create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | cmd_create_flags(usage);
	}

	VkCommandPool pool = {};
	vk_check(vkCreateCommandPool(dptr->vk_device(), &create_info, dptr->vk_allocation_callbacks(), &pool));
	return pool;
}

CmdBufferPoolBase::CmdBufferPoolBase() : _thread_id(concurrent::thread_id()) {
}

CmdBufferPoolBase::CmdBufferPoolBase(DevicePtr dptr, CmdBufferUsage preferred) :
		DeviceLinked(dptr),
		_pool(create_pool(dptr, preferred)),
		_usage(preferred),
		_thread_id(concurrent::thread_id()) {
}

CmdBufferPoolBase::~CmdBufferPoolBase() {
	if(device()) {
		if(_fences.size() != _cmd_buffers.size()) {
			y_fatal("CmdBuffers are still in use (% fences, % buffers).", _fences.size(), _cmd_buffers.size());
		}
		join_all();
		_cmd_buffers.clear();
		destroy(_pool);
	}
}

VkCommandPool CmdBufferPoolBase::vk_pool() const {
	return _pool;
}

void CmdBufferPoolBase::join_all() {
	if(_fences.is_empty()/* || _usage == CmdBufferUsage::Secondary*/) {
		return;
	}

	if(vkWaitForFences(device()->vk_device(), _fences.size(), _fences.data(), true, u64(-1)) != VK_SUCCESS) {
		y_fatal("Unable to join fences.");
	}
}

void CmdBufferPoolBase::release(CmdBufferData&& data) {
	y_profile();

	if(data.pool() != this) {
		y_fatal("CmdBufferData was not returned to its original pool.");
	}
	data.release_resources();
	const auto lock = y_profile_unique_lock(_lock);
	_cmd_buffers.push_back(std::move(data));
}

std::unique_ptr<CmdBufferDataProxy> CmdBufferPoolBase::alloc() {
	y_profile();

	y_debug_assert(concurrent::thread_id() == _thread_id);
	{
		auto lock = y_profile_unique_lock(_lock);
		if(!_cmd_buffers.is_empty()) {
			CmdBufferData data = _cmd_buffers.pop();
			lock.unlock();

			data.reset();
			return std::make_unique<CmdBufferDataProxy>(std::move(data));
		}
	}

	return std::make_unique<CmdBufferDataProxy>(create_data());
}

CmdBufferData CmdBufferPoolBase::create_data() {
	const VkFenceCreateInfo fence_create_info = vk_struct();
	VkCommandBufferAllocateInfo allocate_info = vk_struct();
	{
		allocate_info.commandBufferCount = 1;
		allocate_info.commandPool = _pool;
		allocate_info.level = cmd_level(_usage);
	}

	VkCommandBuffer buffer = {};
	VkFence fence = {};
	vk_check(vkAllocateCommandBuffers(device()->vk_device(), &allocate_info, &buffer));
	vk_check(vkCreateFence(device()->vk_device(), &fence_create_info, device()->vk_allocation_callbacks(), &fence));


	_fences << fence;
	return CmdBufferData(buffer, fence, this);
}

}
