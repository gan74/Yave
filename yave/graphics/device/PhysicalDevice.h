/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef YAVE_DEVICE_PHYSICALDEVICE_H
#define YAVE_DEVICE_PHYSICALDEVICE_H

#include "DeviceProperties.h"

#include <yave/graphics/vk/vk.h>

#include <y/core/Vector.h>

#include <string_view>

namespace yave {

class PhysicalDevice {
    public:
        VkPhysicalDevice vk_physical_device() const;

        DeviceProperties device_properties() const;

        const VkPhysicalDeviceProperties& vk_properties() const;
        const VkPhysicalDeviceVulkan11Properties& vk_properties_1_1() const;
        const VkPhysicalDeviceVulkan12Properties& vk_properties_1_2() const;

        const VkPhysicalDeviceInlineUniformBlockPropertiesEXT& vk_uniform_block_properties() const;

        const VkPhysicalDeviceMemoryProperties& vk_memory_properties() const;

        bool is_discrete() const;
        usize total_device_memory() const;

        bool support_features(const VkPhysicalDeviceFeatures& features) const;
        bool support_features(const VkPhysicalDeviceVulkan11Features& features) const;
        bool support_features(const VkPhysicalDeviceVulkan12Features& features) const;

        core::Vector<VkExtensionProperties> supported_extensions() const;
        bool is_extension_supported(std::string_view name) const;

    private:
        friend class Instance;

        PhysicalDevice(VkPhysicalDevice device);

        VkPhysicalDevice _device = {};
        VkPhysicalDeviceMemoryProperties _memory_properties = {};

        VkPhysicalDeviceProperties2 _properties = vk_struct();
        VkPhysicalDeviceVulkan11Properties _properties_1_1 = vk_struct();
        VkPhysicalDeviceVulkan12Properties _properties_1_2 = vk_struct();

        VkPhysicalDeviceFeatures2 _supported_features = vk_struct();
        VkPhysicalDeviceVulkan11Features _supported_features_1_1 = vk_struct();
        VkPhysicalDeviceVulkan12Features _supported_features_1_2 = vk_struct();

        VkPhysicalDeviceInlineUniformBlockPropertiesEXT _uniform_blocks_properties = vk_struct();
};

}

#endif // YAVE_DEVICE_PHYSICALDEVICE_H

