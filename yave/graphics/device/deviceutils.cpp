/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include <yave/graphics/buffers/Buffer.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <bit>

namespace yave {

core::Span<const char*> raytracing_extensions() {
    static constexpr std::array extensions = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
    };

    return extensions;
}

core::Span<const char*> validation_extensions() {
    static constexpr std::array extensions = {
        "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor"
    };

    return extensions;
}

float device_score(const PhysicalDevice& device) {
    if(device.vulkan_version() < required_vulkan_version()) {
        return 0.0f;
    }

    if(!has_required_features(device)) {
        return 0.0f;
    }

    if(!has_required_properties(device)) {
        return 0.0f;
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
        case SamplerType::LinearRepeatAniso:
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
        case SamplerType::LinearRepeatAniso:
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
        case SamplerType::LinearRepeatAniso:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;

        default:
            y_fatal("Unknown sampler type");
    }
}

VkDeviceAddress vk_buffer_device_address(const SubBufferBase& buffer) {
    VkBufferDeviceAddressInfo info = vk_struct();
    info.buffer = buffer.vk_buffer();

    return vkGetBufferDeviceAddress(vk_device(), &info) + buffer.byte_offset();
}

VkHandle<VkSampler> create_sampler(SamplerType type) {

    VkSamplerCreateInfo create_info = vk_struct();
    {
        const VkSamplerAddressMode address_mode = vk_address_mode(type);
        const VkFilter filter = vk_filter(type);
        const VkSamplerMipmapMode mip_filter = vk_mip_filter(type);

        const float anisotropy = sampler_anisotropy(type);

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
    const u32 version = properties.apiVersion;
    log_msg(fmt("Running Vulkan ({}.{}.{}) {} bits on {} ({})", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version),
            is_64_bits() ? 64 : 32, properties.deviceName, (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete" : "integrated")));
}

void print_enabled_extensions(core::Span<const char*> extensions) {
    log_msg(fmt("{} extensions enabled", extensions.size()));
    for(const char* ext : extensions) {
        log_msg(fmt("  - {}", ext));
    }
}

void print_properties(const DeviceProperties& properties) {
    log_msg(fmt("max_memory_allocations = {}", properties.max_memory_allocations));
    log_msg(fmt("max_uniform_buffer_size = {}", properties.max_uniform_buffer_size));
    log_msg(fmt("raytracing support = {}", properties.has_raytracing));
}


PhysicalDevice find_best_device(const Instance& instance) {
    const auto devices = instance.physical_devices();

    float best_score = -std::numeric_limits<float>::max();
    usize device_index = usize(-1);

    for(usize i = 0; i != devices.size(); ++i) {
        const float dev_score = device_score(devices[i]);

        const u32 version = devices[i].vulkan_version();
        log_msg(fmt("{} with VK {}.{}.{}, score: {}", devices[i].vk_properties().deviceName, VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version), dev_score), Log::Debug);

        if(dev_score > 0.0f && dev_score > best_score) {
            best_score = dev_score;
            device_index = i;
        }
    }

    y_always_assert(device_index != usize(-1), "No compatible device found");

    return devices[device_index];
}

u32 required_vulkan_version() {
    return VK_API_VERSION_1_4;
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
        required.shaderDrawParameters = true;
    }

    return required;
}

VkPhysicalDeviceVulkan12Features required_device_features_1_2() {
    VkPhysicalDeviceVulkan12Features required = vk_struct();

    required.bufferDeviceAddress = true;
    required.timelineSemaphore = true;
    required.runtimeDescriptorArray = true;
    required.descriptorIndexing = true;
    required.descriptorBindingVariableDescriptorCount = true;
    required.descriptorBindingPartiallyBound = true;
    required.descriptorBindingUpdateUnusedWhilePending = true;
    required.descriptorBindingSampledImageUpdateAfterBind = true;
    required.descriptorBindingStorageBufferUpdateAfterBind = true;
    required.shaderSampledImageArrayNonUniformIndexing = true;
    required.shaderStorageBufferArrayNonUniformIndexing = true;

    return required;
}

VkPhysicalDeviceVulkan13Features required_device_features_1_3() {
    VkPhysicalDeviceVulkan13Features required = vk_struct();

    required.maintenance4 = true;

    return required;
}

VkPhysicalDeviceVulkan14Features required_device_features_1_4() {
    VkPhysicalDeviceVulkan14Features required = vk_struct();

    required.pushDescriptor = true;
    required.maintenance5 = true;

    return required;
}

VkPhysicalDeviceAccelerationStructureFeaturesKHR required_device_features_accel_struct() {
    VkPhysicalDeviceAccelerationStructureFeaturesKHR required = vk_struct();

    required.accelerationStructure = true;

    return required;
}

VkPhysicalDeviceRayTracingPipelineFeaturesKHR required_device_features_raytracing_pipeline() {
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR required = vk_struct();

    required.rayTracingPipeline = true;

    return required;
}


VkPhysicalDeviceRayQueryFeaturesKHR required_device_features_ray_query() {
    VkPhysicalDeviceRayQueryFeaturesKHR required = vk_struct();

    required.rayQuery = true;

    return required;
}


bool has_required_features(const PhysicalDevice& physical) {
    if(!physical.supports_features(required_device_features())) {
        return false;
    }

    if(!physical.supports_features(required_device_features_1_1())) {
        return false;
    }

    if(!physical.supports_features(required_device_features_1_2())) {
        return false;
    }

    if(!physical.supports_features(required_device_features_1_3())) {
        return false;
    }

    if(!physical.supports_features(required_device_features_1_4())) {
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

