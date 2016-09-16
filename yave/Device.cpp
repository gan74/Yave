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

Device::Device(Instance& instance) : _instance(instance), _physical(instance) {
	_extentions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	compute_queue_families();
	create_device();
}

bool Device::are_families_complete() const {
	for(i32 index : _queue_familiy_indices) {
		if(index < 0) {
			return false;
		}
	}
	return true;
}

void Device::compute_queue_families() {
	for(i32& index : _queue_familiy_indices) {
		index = -1;
	}

	auto families = _physical.get_vk_physical_device().getQueueFamilyProperties();
	for(i32 i = 0; i != i32(families.size()); i++) {
		auto family = families[i];
		if(family.queueCount <= 0) {
			continue;
		}

		if(family.queueFlags & vk::QueueFlagBits::eGraphics) {
			_queue_familiy_indices[QueueFamily::Graphics] = i;
		}

		if(are_families_complete()) {
			return;
		}
	}
	if(!are_families_complete()) {
		fatal("Unable to find queue family");
	}
}

void Device::create_device() {
	std::array<vk::DeviceQueueCreateInfo, QueueFamily::Max> queue_create_infos;

	float priorities = 1.0f;
	for(usize i = 0; i != _queues.size(); i++) {
		queue_create_infos[i] = vk::DeviceQueueCreateInfo()
				.setQueueFamilyIndex(_queue_familiy_indices[i])
				.setPQueuePriorities(&priorities)
				.setQueueCount(1)
			;
	}

	auto features = _physical.get_vk_physical_device().getFeatures();
	_device = _physical.get_vk_physical_device().createDevice(vk::DeviceCreateInfo()
			.setEnabledExtensionCount(_extentions.size())
			.setPpEnabledExtensionNames(_extentions.begin())
			.setEnabledLayerCount(_instance.get_debug_params().get_device_layers().size())
			.setPpEnabledLayerNames(_instance.get_debug_params().get_device_layers().begin())
			.setQueueCreateInfoCount(queue_create_infos.size())
			.setPQueueCreateInfos(queue_create_infos.begin())
			.setPEnabledFeatures(&features)
		);

	for(usize i = 0; i != _queues.size(); i++) {
		_queues[i] = _device.getQueue(_queue_familiy_indices[i], 0);
	}

	_cmd_pool = CmdBufferPool(this);
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

	_device.destroy();
}

const PhysicalDevice& Device::get_physical_device() const {
	return _physical;
}

const Instance &Device::get_instance() const {
	return _instance;
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
