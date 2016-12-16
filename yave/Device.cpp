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

#include "Device.h"
#include <yave/commands/CmdBuffer.h>

namespace yave {

static bool are_families_complete(const std::array<u32, QueueFamily::Max>& families) {
	for(u32 index : families) {
		if(index == u32(-1)) {
			return false;
		}
	}
	return true;
}

static std::array<u32, QueueFamily::Max> compute_queue_families(vk::PhysicalDevice physical) {
	std::array<u32, QueueFamily::Max> queue_families;
	for(u32& index : queue_families) {
		index = u32(-1);
	}

	auto families = physical.getQueueFamilyProperties();
	for(u32 i = 0; i != families.size(); i++) {
		auto family = families[usize(i)];
		if(family.queueCount <= 0) {
			continue;
		}

		if(family.queueFlags & vk::QueueFlagBits::eGraphics) {
			queue_families[QueueFamily::Graphics] = i;
		}

		if(are_families_complete(queue_families)) {
			return queue_families;
		}
	}
	if(!are_families_complete(queue_families)) {
		fatal("Unable to find queue family");
	}
	return queue_families;
}

static vk::Device create_device(
		vk::PhysicalDevice physical,
		const std::array<u32, QueueFamily::Max>& queue_families,
		const core::Vector<const char*>& extentions,
		const core::Vector<const char*>& layers) {

	std::array<vk::DeviceQueueCreateInfo, QueueFamily::Max> queue_create_infos;

	float priorities = 1.0f;
	for(usize i = 0; i != QueueFamily::Max; i++) {
		queue_create_infos[i] = vk::DeviceQueueCreateInfo()
				.setQueueFamilyIndex(queue_families[i])
				.setPQueuePriorities(&priorities)
				.setQueueCount(1)
			;
	}


	auto features = physical.getFeatures();

	if(!features.multiDrawIndirect) {
		fatal("Multi-draw-indirect required.");
	}

	return physical.createDevice(vk::DeviceCreateInfo()
			.setEnabledExtensionCount(u32(extentions.size()))
			.setPpEnabledExtensionNames(extentions.begin())
			.setEnabledLayerCount(u32(layers.size()))
			.setPpEnabledLayerNames(layers.begin())
			.setQueueCreateInfoCount(queue_create_infos.size())
			.setPQueueCreateInfos(queue_create_infos.begin())
			.setPEnabledFeatures(&features)
		);
}


Device::Device(Instance& instance) :
		_instance(instance),
		_physical(instance),
		_queue_familiy_indices(compute_queue_families(_physical.vk_physical_device())),
		_device(create_device(_physical.vk_physical_device(), _queue_familiy_indices, {VK_KHR_SWAPCHAIN_EXTENSION_NAME}, _instance.debug_params().device_layers())),
		_sampler(this),
		_disposable_cmd_pool(this),
		_descriptor_layout_pool(new DescriptorSetLayoutPool(this)) {

	for(usize i = 0; i != _queues.size(); i++) {
		_queues[i] = _device.getQueue(_queue_familiy_indices[i], 0);
	}
}

Device::~Device() {
	for(auto q : _queues) {
		q.waitIdle();
	}

	if(_disposable_cmd_pool.active_buffers()) {
		fatal("Buffer still active");
	}

	_sampler = Sampler();

	// we need to destroy the pools before the device
	_disposable_cmd_pool = CmdBufferPool();
	delete _descriptor_layout_pool;

	_device.destroy();
}

const PhysicalDevice& Device::physical_device() const {
	return _physical;
}

const Instance &Device::instance() const {
	return _instance;
}

vk::Device Device::vk_device() const {
	return _device;
}

vk::Queue Device::vk_queue(usize i) const {
	return _queues[i];
}

vk::Sampler Device::vk_sampler() const {
	return _sampler.vk_sampler();
}

u32 Device::queue_family_index(QueueFamily i) const {
	return u32(_queue_familiy_indices[i]);
}

}
