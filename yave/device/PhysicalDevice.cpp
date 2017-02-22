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

#include "PhysicalDevice.h"


namespace yave {

static bool is_device_ok(vk::PhysicalDevice device) {
	auto features = device.getFeatures();
	return  features.geometryShader&&
			features.fullDrawIndexUint32&&
			features.robustBufferAccess;
}

static vk::PhysicalDevice choose_device(vk::Instance instance) {
	for(auto dev : instance.enumeratePhysicalDevices()) {
		if(is_device_ok(dev)) {
			return dev;
		}
	}
	return fatal("Unable to find a compatible device.");
}




PhysicalDevice::PhysicalDevice(Instance& instance) :
		_instance(instance),
		_device(choose_device(instance.vk_instance())),
		_memory_properties(_device.getMemoryProperties()) {

	auto properties = _device.getProperties();
	const auto& v_ref = properties.apiVersion;
	auto version = reinterpret_cast<const Version&>(v_ref);
	log_msg(core::String() + "Running Vulkan (" + version.major + "." + version.minor + "." + version.patch + ") " + (is_64_bits() ? 64 : 32) + " bits" +
				 " on " + properties.deviceName + " (" + (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? "discrete" : "integrated") + ")");
}

PhysicalDevice::~PhysicalDevice() {
}

vk::PhysicalDevice PhysicalDevice::vk_physical_device() const {
	return _device;
}

vk::PhysicalDeviceMemoryProperties PhysicalDevice::vk_memory_properties() const {
	return _memory_properties;
}

}
