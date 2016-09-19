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
#include <yave/command/CmdBuffer.h>

namespace yave {

bool are_families_complete(const std::array<i32, QueueFamily::Max>& families) {
	for(i32 index : families) {
		if(index < 0) {
			return false;
		}
	}
	return true;
}

std::array<i32, QueueFamily::Max> compute_queue_families(vk::PhysicalDevice physical) {
	std::array<i32, QueueFamily::Max> queue_families;
	for(i32& index : queue_families) {
		index = -1;
	}

	auto families = physical.getQueueFamilyProperties();
	for(i32 i = 0; i != i32(families.size()); i++) {
		auto family = families[i];
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

vk::Device create_device(
		vk::PhysicalDevice physical,
		const std::array<i32, QueueFamily::Max>& queue_families,
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
	return physical.createDevice(vk::DeviceCreateInfo()
			.setEnabledExtensionCount(extentions.size())
			.setPpEnabledExtensionNames(extentions.begin())
			.setEnabledLayerCount(layers.size())
			.setPpEnabledLayerNames(layers.begin())
			.setQueueCreateInfoCount(queue_create_infos.size())
			.setPQueueCreateInfos(queue_create_infos.begin())
			.setPEnabledFeatures(&features)
		);
}


Device::Device(Instance& instance) :
		_instance(instance),
		_physical(instance),
		_queue_familiy_indices(compute_queue_families(_physical.get_vk_physical_device())),
		_device(create_device(_physical.get_vk_physical_device(), _queue_familiy_indices, {VK_KHR_SWAPCHAIN_EXTENSION_NAME}, _instance.get_debug_params().get_device_layers())),
		_cmd_pool(this),
		_ds_builder(this) {

	for(usize i = 0; i != _queues.size(); i++) {
		_queues[i] = _device.getQueue(_queue_familiy_indices[i], 0);
	}
}

Device::~Device() {
	for(auto q : _queues) {
		q.waitIdle();
	}

	if(_cmd_pool.active_buffers()) {
		fatal("Buffer still active");
	}

	// we need to destroy the pool before the device
	_cmd_pool = CmdBufferPool();
	_ds_builder = DescriptorSetBuilder();

	_device.destroy();
}

const PhysicalDevice& Device::get_physical_device() const {
	return _physical;
}

const Instance &Device::get_instance() const {
	return _instance;
}

const DescriptorSetBuilder& Device::get_deescriptor_set_builder() const {
	return _ds_builder;
}

vk::Device Device::get_vk_device() const {
	return _device;
}

vk::Queue Device::get_vk_queue(usize i) const {
	return _queues[i];
}

i32 Device::get_queue_family_index(QueueFamily i) const {
	return _queue_familiy_indices[i];
}

}
