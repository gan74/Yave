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
#ifndef YAVE_DEVICE_DEVICEUTILS_H
#define YAVE_DEVICE_DEVICEUTILS_H

#include "Device.h"
#include "PhysicalDevice.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

// https://stackoverflow.com/questions/16190078/how-to-atomically-update-a-maximum-value
template<typename T>
static void update_maximum(std::atomic<T>& maximum_value, const T& value) noexcept {
    T prev_value = maximum_value;
    while(prev_value < value && !maximum_value.compare_exchange_weak(prev_value, value)) {
    }
}

static float device_score(const PhysicalDevice& device) {
    if(!Device::has_required_features(device)) {
        return -std::numeric_limits<float>::max();
    }

    if(!Device::has_required_properties(device)) {
        return -std::numeric_limits<float>::max();
    }

    const usize heap_size = device.total_device_memory() / (1024 * 1024);
    const float heap_score = float(heap_size) / float(heap_size + 8 * 1024);
    const float type_score = device.is_discrete() ? 1.0f : 0.0f;
    return heap_score + type_score;
}


static bool try_enable_extension(core::Vector<const char*>& exts, const char* name, const PhysicalDevice& device) {
    if(device.is_extension_supported(name)) {
        exts << name;
        return true;
    }
    log_msg(fmt("% not supported", name), Log::Warning);
    return false;
}

static VkSamplerAddressMode vk_address_mode(SamplerType type) {
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

static VkFilter vk_filter(SamplerType type) {
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

static VkSamplerMipmapMode vk_mip_filter(SamplerType type) {
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

static VkSampler create_sampler(DevicePtr dptr, SamplerType type) {

    VkSamplerCreateInfo create_info = vk_struct();
    {
        const VkSamplerAddressMode address_mode = vk_address_mode(type);
        const VkFilter filter = vk_filter(type);
        const VkSamplerMipmapMode mip_filter = vk_mip_filter(type);

        create_info.addressModeU = address_mode;
        create_info.addressModeV = address_mode;
        create_info.addressModeW = address_mode;
        create_info.magFilter = filter;
        create_info.minFilter = filter;
        create_info.mipmapMode = mip_filter;
        create_info.maxLod = 1000.0f;
        create_info.maxAnisotropy = 1.0f;
        create_info.compareEnable = type == SamplerType::Shadow;
        create_info.compareOp = VK_COMPARE_OP_GREATER;
    }

    VkSampler sampler = {};
    vk_check(vkCreateSampler(dptr->vk_device(), &create_info, dptr->vk_allocation_callbacks(), &sampler));
    return sampler;
}

static core::Vector<VkQueueFamilyProperties> enumerate_family_properties(VkPhysicalDevice device) {
    core::Vector<VkQueueFamilyProperties> families;
    {
        u32 count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        families = core::Vector<VkQueueFamilyProperties>(count, VkQueueFamilyProperties{});
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
    }
    return families;
}

static u32 queue_family_index(core::Span<VkQueueFamilyProperties> families, VkQueueFlags flags) {
    for(usize i = 0; i != families.size(); ++i) {
        if(families[i].queueCount && (families[i].queueFlags & flags) == flags) {
            return u32(i);
        }
    }
    y_fatal("No queue available for given flag set");
}

static VkQueue create_queue(VkDevice device, u32 family_index, u32 index) {
    VkQueue q = {};
    vkGetDeviceQueue(device, family_index, index, &q);
    return q;
}

static void print_physical_properties(const VkPhysicalDeviceProperties& properties) {
    struct Version {
        const u32 patch : 12;
        const u32 minor : 10;
        const u32 major : 10;
    };

    const auto& v_ref = properties.apiVersion;
    const auto version = reinterpret_cast<const Version&>(v_ref);
    log_msg(fmt("Running Vulkan (%.%.%) % bits on % (%)", u32(version.major), u32(version.minor), u32(version.patch),
            is_64_bits() ? 64 : 32, properties.deviceName, (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete" : "integrated")));
    log_msg(fmt("sizeof(PhysicalDevice) = %", sizeof(PhysicalDevice)));
}

static void print_enabled_extensions(core::Span<const char*> extensions) {
    for(const char* ext : extensions) {
        log_msg(fmt("% enabled", ext), Log::Debug);
    }
}

static void print_properties(const DeviceProperties& properties) {
    log_msg(fmt("max_memory_allocations = %", properties.max_memory_allocations), Log::Debug);
    log_msg(fmt("max_inline_uniform_size = %", properties.max_inline_uniform_size), Log::Debug);
    log_msg(fmt("max_uniform_buffer_size = %", properties.max_uniform_buffer_size), Log::Debug);
}





}


#endif // YAVE_DEVICE_DEVICEUTILS_H

