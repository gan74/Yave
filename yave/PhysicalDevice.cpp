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
	return fatal("Unable to find compatible device");
}




PhysicalDevice::PhysicalDevice(Instance& instance) :
		_instance(instance),
		_device(choose_device(instance.get_vk_instance())),
		_memory_properties(_device.getMemoryProperties()) {

	auto properties = _device.getProperties();
	const auto& v_ref = properties.apiVersion;
	auto version = reinterpret_cast<const Version&>(v_ref);
	log_msg(core::String() + "Running Vulkan (" + version.major + "." + version.minor + "." + version.patch + ") " + (is_64_bits() ? 64 : 32) + " bits" +
				 " on " + properties.deviceName + " (" + (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? "discrete" : "integrated") + ")");
}

PhysicalDevice::~PhysicalDevice() {
}

vk::PhysicalDevice PhysicalDevice::get_vk_physical_device() const {
	return _device;
}

vk::PhysicalDeviceMemoryProperties PhysicalDevice::get_vk_memory_properties() const {
	return _memory_properties;
}

}
