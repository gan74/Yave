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

#include "deviceutils.h"

#include "Instance.h"
#include "PhysicalDevice.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <bit>

namespace yave {

float device_score(const PhysicalDevice& device) {
    if(!has_required_features(device)) {
        return -std::numeric_limits<float>::max();
    }

    if(!has_required_properties(device)) {
        return -std::numeric_limits<float>::max();
    }

    const usize heap_size = device.total_device_memory() / (1024 * 1024);
    const float heap_score = float(heap_size) / float(heap_size + 8 * 1024);
    const float type_score = device.is_discrete() ? 1.0f : 0.0f;
    return heap_score + type_score;
}


bool try_enable_extension(core::Vector<const char*>& exts, const char* name, const PhysicalDevice& device) {
    if(device.is_extension_supported(name)) {
        exts << name;
        return true;
    }
    log_msg(fmt("{} not supported", name), Log::Warning);
    return false;
}

VkSamplerAddressMode vk_address_mode(SamplerType type) {
    switch(type) {
        case SamplerType::LinearRepeat:
        case SamplerType::PointRepeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;

        case SamplerType::Shadow:
        case SamplerType::LinearClamp:
        case SamplerType::PointClamp:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        default:
            y_fatal("Unknown sampler type");
    }
}

VkFilter vk_filter(SamplerType type) {
    switch(type) {
        case SamplerType::PointRepeat:
        case SamplerType::PointClamp:
            return VK_FILTER_NEAREST;

        case SamplerType::Shadow:
        case SamplerType::LinearRepeat:
        case SamplerType::LinearClamp:
            return VK_FILTER_LINEAR;

        default:
            y_fatal("Unknown sampler type");
    }
}

VkSamplerMipmapMode vk_mip_filter(SamplerType type) {
    switch(type) {
        case SamplerType::PointRepeat:
        case SamplerType::PointClamp:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case SamplerType::Shadow:
        case SamplerType::LinearRepeat:
        case SamplerType::LinearClamp:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;

        default:
            y_fatal("Unknown sampler type");
    }
}

VkHandle<VkSampler> create_sampler(SamplerType type) {

    VkSamplerCreateInfo create_info = vk_struct();
    {
        const VkSamplerAddressMode address_mode = vk_address_mode(type);
        const VkFilter filter = vk_filter(type);
        const VkSamplerMipmapMode mip_filter = vk_mip_filter(type);

        const float anisotropy = 1.0f;

        create_info.addressModeU = address_mode;
        create_info.addressModeV = address_mode;
        create_info.addressModeW = address_mode;
        create_info.magFilter = filter;
        create_info.minFilter = filter;
        create_info.mipmapMode = mip_filter;
        create_info.maxLod = 1000.0f;
        create_info.anisotropyEnable = anisotropy > 1.0f;
        create_info.maxAnisotropy = anisotropy;
        create_info.compareEnable = type == SamplerType::Shadow;
        create_info.compareOp = VK_COMPARE_OP_GREATER;
    }

    VkHandle<VkSampler> sampler;
    vk_check(vkCreateSampler(vk_device(), &create_info, vk_allocation_callbacks(), sampler.get_ptr_for_init()));
    return sampler;
}

core::Vector<VkQueueFamilyProperties> enumerate_family_properties(VkPhysicalDevice device) {
    core::Vector<VkQueueFamilyProperties> families;
    {
        u32 count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        families = core::Vector<VkQueueFamilyProperties>(count, VkQueueFamilyProperties{});
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
    }
    return families;
}

u32 queue_family_index(core::Span<VkQueueFamilyProperties> families, VkQueueFlags flags) {
    int most_bits = -1;
    usize best_family_index = 0;

    for(usize i = 0; i != families.size(); ++i) {
        if(!families[i].queueCount) {
            continue;
        }

        if((families[i].queueFlags & flags) != flags) {
            continue;
        }

        const int bits = std::popcount(families[i].queueFlags);
        if(most_bits < 0 || bits < most_bits) {
            most_bits = bits;
            best_family_index = i;
        }
    }

    y_always_assert(most_bits >= 0, "No queue available for given flag set");
    return u32(best_family_index);
}

VkQueue create_queue(VkDevice device, u32 family_index, u32 index) {
    VkQueue q = {};
    vkGetDeviceQueue(device, family_index, index, &q);
    return q;
}

void print_physical_properties(const VkPhysicalDeviceProperties& properties) {
    struct Version {
        const u32 patch : 12;
        const u32 minor : 10;
        const u32 major : 10;
    };

    const Version version = std::bit_cast<Version>(properties.apiVersion);
    log_msg(fmt("Running Vulkan ({}.{}.{}) {} bits on {} ({})", u32(version.major), u32(version.minor), u32(version.patch),
            is_64_bits() ? 64 : 32, properties.deviceName, (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete" : "integrated")));
}

void print_enabled_extensions(core::Span<const char*> extensions) {
    for(const char* ext : extensions) {
        log_msg(fmt("{} enabled", ext));
    }
}

void print_properties(const DeviceProperties& properties) {
    log_msg(fmt("max_memory_allocations = {}", properties.max_memory_allocations));
    log_msg(fmt("max_inline_uniform_size = {}", properties.max_inline_uniform_size));
    log_msg(fmt("max_uniform_buffer_size = {}", properties.max_uniform_buffer_size));
}


PhysicalDevice find_best_device(const Instance& instance) {
    const auto devices = instance.physical_devices();

    float best_score = -std::numeric_limits<float>::max();
    usize device_index = usize(-1);

    for(usize i = 0; i != devices.size(); ++i) {
        const float dev_score = device_score(devices[i]);
        if(dev_score > best_score) {
            best_score = dev_score;
            device_index = i;
        }
    }

    y_always_assert(device_index != usize(-1), "No compatible device found");

    return devices[device_index];
}

VkPhysicalDeviceFeatures required_device_features() {
    VkPhysicalDeviceFeatures required = {};

    {
        required.multiDrawIndirect = true;
        required.geometryShader = true;
        required.drawIndirectFirstInstance = true;
        required.fullDrawIndexUint32 = true;
        required.textureCompressionBC = true;
        required.shaderStorageImageExtendedFormats = true;
        required.shaderUniformBufferArrayDynamicIndexing = true;
        required.shaderSampledImageArrayDynamicIndexing = true;
        required.shaderStorageBufferArrayDynamicIndexing = true;
        required.shaderStorageImageArrayDynamicIndexing = true;
        required.fragmentStoresAndAtomics = true;
        required.independentBlend = true;
        required.samplerAnisotropy = true;
    }

    return required;
}

VkPhysicalDeviceVulkan11Features required_device_features_1_1() {
    VkPhysicalDeviceVulkan11Features required = vk_struct();

    {
        // We don't actually need anything here...
    }

    return required;
}

VkPhysicalDeviceVulkan12Features required_device_features_1_2() {
    VkPhysicalDeviceVulkan12Features required = vk_struct();

    {
        required.timelineSemaphore = true;
        required.runtimeDescriptorArray = true;
        required.descriptorIndexing = true;
        required.descriptorBindingVariableDescriptorCount = true;
        required.descriptorBindingPartiallyBound = true;
        required.descriptorBindingUpdateUnusedWhilePending = true;
        required.descriptorBindingSampledImageUpdateAfterBind = true;
        required.shaderSampledImageArrayNonUniformIndexing = true;
    }

    return required;
}

VkPhysicalDeviceVulkan13Features required_device_features_1_3() {
    VkPhysicalDeviceVulkan13Features required = vk_struct();

    {
        // required.inlineUniformBlock = true; // Fallback
    }

    return required;
}

bool has_required_features(const PhysicalDevice& physical) {
    if(!physical.support_features(required_device_features())) {
        return false;
    }

    if(!physical.support_features(required_device_features_1_1())) {
        return false;
    }

    if(!physical.support_features(required_device_features_1_2())) {
        return false;
    }

    if(!physical.support_features(required_device_features_1_3())) {
        return false;
    }

    return true;
}

bool has_required_properties(const PhysicalDevice &physical) {
    const auto& p11 = physical.vk_properties_1_1();

    bool ok = true;

    ok &= !!p11.subgroupQuadOperationsInAllStages;

    return ok;
}

}

