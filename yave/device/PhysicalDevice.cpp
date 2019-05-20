/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include "PhysicalDevice.h"

namespace yave {

static bool is_device_ok(vk::PhysicalDevice device) {
	return device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
}

static vk::PhysicalDevice choose_device(vk::Instance instance) {
	for(auto dev : instance.enumeratePhysicalDevices()) {
		if(is_device_ok(dev)) {
			return dev;
		}
	}
	return y_fatal("Unable to find a compatible device.");
}



PhysicalDevice::PhysicalDevice(Instance& instance) :
		_instance(instance),
		_device(choose_device(instance.vk_instance())),
		_properties(_device.getProperties()),
		_memory_properties(_device.getMemoryProperties()) {

	struct Version {
		u32 patch : 12;
		u32 minor : 10;
		u32 major : 10;
	};

	const auto& v_ref = _properties.apiVersion;
	auto version = reinterpret_cast<const Version&>(v_ref);
	log_msg(fmt("Running Vulkan (%.%.%) % bits on % (%)", u32(version.major), u32(version.minor), u32(version.patch),
			is_64_bits() ? 64 : 32, _properties.deviceName, (_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? "discrete" : "integrated")));
}

PhysicalDevice::~PhysicalDevice() {
}

vk::PhysicalDevice PhysicalDevice::vk_physical_device() const {
	return _device;
}

const vk::PhysicalDeviceProperties& PhysicalDevice::vk_properties() const {
	return _properties;
}

const vk::PhysicalDeviceMemoryProperties& PhysicalDevice::vk_memory_properties() const {
	return _memory_properties;
}

}
