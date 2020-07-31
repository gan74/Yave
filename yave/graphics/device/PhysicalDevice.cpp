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

#include "PhysicalDevice.h"

#include <y/core/Vector.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

static bool is_device_ok(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(device, &properties);
    return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

static VkPhysicalDevice choose_device(VkInstance instance) {

    core::Vector<VkPhysicalDevice> devices;
    {
        u32 count = 0;
        vk_check(vkEnumeratePhysicalDevices(instance, &count, nullptr));
        devices = core::Vector<VkPhysicalDevice>(count, VkPhysicalDevice{});
        vk_check(vkEnumeratePhysicalDevices(instance, &count, devices.data()));
    }

    if(devices.is_empty()) {
        y_fatal("Unable to find a compatible device.");
    }
    for(auto dev : devices) {
        if(is_device_ok(dev)) {
            return dev;
        }
    }
    return devices[0];
}



PhysicalDevice::PhysicalDevice(Instance& instance) :
        _instance(instance),
        _device(choose_device(instance.vk_instance())) {

    vkGetPhysicalDeviceMemoryProperties(_device, &_memory_properties);

    {
        _uniform_blocks_properties = vk_struct();

        VkPhysicalDeviceProperties2 properties = vk_struct();
        properties.pNext = &_uniform_blocks_properties;
        vkGetPhysicalDeviceProperties2(_device, &properties);
        _properties = properties.properties;
    }


    struct Version {
        const u32 patch : 12;
        const u32 minor : 10;
        const u32 major : 10;
    };

    const auto& v_ref = _properties.apiVersion;
    const auto version = reinterpret_cast<const Version&>(v_ref);
    log_msg(fmt("Running Vulkan (%.%.%) % bits on % (%)", u32(version.major), u32(version.minor), u32(version.patch),
            is_64_bits() ? 64 : 32, _properties.deviceName, (_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete" : "integrated")));
}

PhysicalDevice::~PhysicalDevice() {
}

VkPhysicalDevice PhysicalDevice::vk_physical_device() const {
    return _device;
}

const VkPhysicalDeviceProperties& PhysicalDevice::vk_properties() const {
    return _properties;
}

const VkPhysicalDeviceInlineUniformBlockPropertiesEXT& PhysicalDevice::vk_uniform_block_properties() const {
    return _uniform_blocks_properties;
}

const VkPhysicalDeviceMemoryProperties& PhysicalDevice::vk_memory_properties() const {
    return _memory_properties;
}

}

