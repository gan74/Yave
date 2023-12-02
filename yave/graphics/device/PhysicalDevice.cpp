/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

template<typename T>
static bool support_all_features(const T& features, const T& supported_features) {
    Y_TODO(VkPhysicalDeviceVulkan11Features have sType and pNext, which may cause trouble here)
    static_assert(sizeof(T) % sizeof(VkBool32) == 0);

    const VkBool32* supported = reinterpret_cast<const VkBool32*>(&supported_features);
    const VkBool32* required = reinterpret_cast<const VkBool32*>(&features);
    for(usize i = 0; i != sizeof(features) / sizeof(VkBool32); ++i) {
        if(required[i] && !supported[i]) {
            return false;
        }
    }
    return true;
}



PhysicalDevice::PhysicalDevice(VkPhysicalDevice device) : _device(device) {
    vkGetPhysicalDeviceMemoryProperties(_device, &_memory_properties);

    {
        _supported_features.pNext = &_supported_features_1_1;
        _supported_features_1_1.pNext = &_supported_features_1_2;
        _supported_features_1_2.pNext = &_supported_features_1_3;

        vkGetPhysicalDeviceFeatures2(_device, &_supported_features);
    }

    {
        _properties.pNext = &_properties_1_1;
        _properties_1_1.pNext = &_properties_1_2;
        _properties_1_2.pNext = &_properties_1_3;

        vkGetPhysicalDeviceProperties2(_device, &_properties);
    }
}

DeviceProperties PhysicalDevice::device_properties() const {
    const VkPhysicalDeviceLimits& limits = vk_properties().limits;

    DeviceProperties properties = {};

    properties.non_coherent_atom_size = limits.nonCoherentAtomSize;
    properties.max_uniform_buffer_size = limits.maxUniformBufferRange;
    properties.uniform_buffer_alignment = limits.minUniformBufferOffsetAlignment;
    properties.storage_buffer_alignment = limits.minStorageBufferOffsetAlignment;

    properties.max_memory_allocations = limits.maxMemoryAllocationCount;

    properties.max_uniform_buffer_desc_array_size = _properties_1_2.maxDescriptorSetUpdateAfterBindUniformBuffers;
    properties.max_storage_buffer_desc_array_size = _properties_1_2.maxDescriptorSetUpdateAfterBindStorageBuffers;
    properties.max_sampled_image_desc_array_size = _properties_1_2.maxDescriptorSetUpdateAfterBindSampledImages;
    properties.max_storage_image_desc_array_size = _properties_1_2.maxDescriptorSetUpdateAfterBindStorageImages;

    properties.max_inline_uniform_size = _properties_1_3.maxInlineUniformBlockSize;

    properties.timestamp_period = limits.timestampPeriod;

    return properties;
}


bool PhysicalDevice::is_discrete() const {
    return vk_properties().deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

u64 PhysicalDevice::total_device_memory() const {
    u64 total = 0;
    for(u32 i = 0; i != _memory_properties.memoryHeapCount; ++i) {
        if(_memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            total += _memory_properties.memoryHeaps[i].size;
        }
    }
    return total;
}

VkPhysicalDevice PhysicalDevice::vk_physical_device() const {
    return _device;
}

const VkPhysicalDeviceMemoryProperties& PhysicalDevice::vk_memory_properties() const {
    return _memory_properties;
}

const VkPhysicalDeviceProperties& PhysicalDevice::vk_properties() const {
    return _properties.properties;
}

const VkPhysicalDeviceVulkan11Properties& PhysicalDevice::vk_properties_1_1() const {
    return _properties_1_1;
}

const VkPhysicalDeviceVulkan12Properties& PhysicalDevice::vk_properties_1_2() const {
    return _properties_1_2;
}

const VkPhysicalDeviceVulkan13Properties& PhysicalDevice::vk_properties_1_3() const {
    return _properties_1_3;
}

bool PhysicalDevice::support_features(const VkPhysicalDeviceFeatures& features) const {
    return support_all_features(features, _supported_features.features);
}

bool PhysicalDevice::support_features(const VkPhysicalDeviceVulkan11Features& features) const {
    return support_all_features(features, _supported_features_1_1);
}

bool PhysicalDevice::support_features(const VkPhysicalDeviceVulkan12Features& features) const {
    return support_all_features(features, _supported_features_1_2);
}

bool PhysicalDevice::support_features(const VkPhysicalDeviceVulkan13Features& features) const {
    return support_all_features(features, _supported_features_1_3);
}

core::Vector<VkExtensionProperties> PhysicalDevice::supported_extensions() const {
    u32 count = 0;
    vk_check(vkEnumerateDeviceExtensionProperties(_device, nullptr, &count, nullptr));
    core::Vector<VkExtensionProperties> ext = core::Vector<VkExtensionProperties>(count, VkExtensionProperties{});
    vk_check(vkEnumerateDeviceExtensionProperties(_device, nullptr, &count, ext.data()));
    return ext;
}

bool PhysicalDevice::is_extension_supported(std::string_view name) const {
    for(const VkExtensionProperties& ext : supported_extensions()) {
        if(name == ext.extensionName) {
            return true;
        }
    }
    return false;
}

}

