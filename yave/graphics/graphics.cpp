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

#include "graphics.h"

#include <yave/graphics/device/deviceutils.h>
#include <yave/graphics/device/Instance.h>
#include <yave/graphics/device/DeviceProperties.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferPool.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/images/Sampler.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/device/LifetimeManager.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/device/DescriptorLayoutAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>
#include <yave/graphics/device/DiagnosticCheckpoints.h>

#include <y/core/ScratchPad.h>

#include <y/utils/log.h>


namespace yave {
namespace device {


Instance* instance = nullptr;
std::unique_ptr<PhysicalDevice> physical_device;

core::Vector<const char*> enabled_extensions;
DeviceProperties device_properties = {};

std::array<Uninitialized<Sampler>, 6> samplers;

Uninitialized<LifetimeManager> lifetime_manager;
Uninitialized<DescriptorLayoutAllocator> layout_allocator;
Uninitialized<MeshAllocator> mesh_allocator;
Uninitialized<MaterialAllocator> material_allocator;
Uninitialized<TextureLibrary> texture_library;
Uninitialized<DeviceResources> resources;

VmaAllocator allocator = {};
VkDevice vk_device = {};

Uninitialized<CmdQueue> queue;

std::unique_ptr<DiagnosticCheckpoints> diagnostic_checkpoints;

#ifdef Y_DEBUG
std::atomic<bool> destroying = false;
#endif

}



static void init_vma() {

    VmaVulkanFunctions vulkan_functions = {};
    {
#define SET_VK_FUNC(func) vulkan_functions.func = func
#define SET_VK_FUNC_KHR(func) vulkan_functions.func ## KHR = func

        SET_VK_FUNC(vkGetPhysicalDeviceProperties);
        SET_VK_FUNC(vkGetPhysicalDeviceMemoryProperties);
        SET_VK_FUNC(vkAllocateMemory);
        SET_VK_FUNC(vkFreeMemory);
        SET_VK_FUNC(vkMapMemory);
        SET_VK_FUNC(vkUnmapMemory);
        SET_VK_FUNC(vkFlushMappedMemoryRanges);
        SET_VK_FUNC(vkInvalidateMappedMemoryRanges);
        SET_VK_FUNC(vkBindBufferMemory);
        SET_VK_FUNC(vkBindImageMemory);
        SET_VK_FUNC(vkGetBufferMemoryRequirements);
        SET_VK_FUNC(vkGetImageMemoryRequirements);
        SET_VK_FUNC(vkCreateBuffer);
        SET_VK_FUNC(vkDestroyBuffer);
        SET_VK_FUNC(vkCreateImage);
        SET_VK_FUNC(vkDestroyImage);
        SET_VK_FUNC(vkCmdCopyBuffer);
        SET_VK_FUNC(vkGetDeviceBufferMemoryRequirements);
        SET_VK_FUNC(vkGetDeviceImageMemoryRequirements);
        SET_VK_FUNC_KHR(vkGetBufferMemoryRequirements2);
        SET_VK_FUNC_KHR(vkGetImageMemoryRequirements2);
        SET_VK_FUNC_KHR(vkBindBufferMemory2);
        SET_VK_FUNC_KHR(vkBindImageMemory2);
        SET_VK_FUNC_KHR(vkGetPhysicalDeviceMemoryProperties2);

#undef SET_VK_FUNC_KHR
#undef SET_VK_FUNC
    }

    VmaAllocatorCreateInfo create_info = {};
    {
        create_info.vulkanApiVersion = VK_API_VERSION_1_2;
        create_info.pVulkanFunctions = &vulkan_functions;
        create_info.physicalDevice = device::physical_device->vk_physical_device();
        create_info.device = device::vk_device;
        create_info.instance = device::instance->vk_instance();
        create_info.pAllocationCallbacks = vk_allocation_callbacks();
        create_info.flags =
            VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT |
            VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT |
            VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT
        ;
    }

    if(is_extension_enabled(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)) {
        create_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    }

    vmaCreateAllocator(&create_info, &device::allocator);
}

static void init_vk_device() {
    y_profile();

    const core::Vector<VkQueueFamilyProperties> queue_families = enumerate_family_properties(vk_physical_device());

    const VkQueueFlags graphic_queue_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    const u32 main_queue_index = queue_family_index(queue_families, graphic_queue_flags);

    // const VkQueueFamilyProperties main_queue_family_properties = queue_families[main_queue_index];
    const usize queue_count = 1;

    device::enabled_extensions.make_empty();
    for(const char* ext : required_extensions()) {
        y_always_assert(try_enable_extension(device::enabled_extensions, ext, physical_device()), "Device doesn't support required extension ({})", ext);
    }

    if(raytracing_enabled()) {
        for(const char* ext_name : raytracing_extensions()) {
            device::enabled_extensions << ext_name;
        }
    }

    try_enable_extension(device::enabled_extensions, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, physical_device());

    const bool diagnostic_checkpoints = instance_params().debug_utils && try_enable_extension(device::enabled_extensions, DiagnosticCheckpoints::extension_name(), physical_device());

    const auto required_features = required_device_features();
    auto required_features_1_1 = required_device_features_1_1();
    auto required_features_1_2 = required_device_features_1_2();
    auto required_features_1_3 = required_device_features_1_3();
    auto required_features_1_4 = required_device_features_1_4();


    auto required_features_accel = required_device_features_accel_struct();
    auto required_features_raytracing = required_device_features_raytracing_pipeline();
    auto required_features_ray_query = required_device_features_ray_query();
    auto required_features_ray_position_fetch = required_device_features_ray_position_fetch();
    auto required_features_shader_float_atomic = required_device_features_shader_float_atomic();

    y_always_assert(has_required_features(physical_device()), "{} doesn't support required features", physical_device().device_name());
    y_always_assert(has_required_properties(physical_device()), "{} doesn't support required properties", physical_device().device_name());

    print_physical_properties(physical_device().vk_properties());
    print_enabled_extensions(device::enabled_extensions);

    core::ScratchPad<float> queue_priorities(queue_count);
    std::fill_n(queue_priorities.data(), queue_priorities.size(), 0.0f);
    queue_priorities[0] = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info = vk_struct();
    {
        queue_create_info.queueFamilyIndex = main_queue_index;
        queue_create_info.pQueuePriorities = queue_priorities.data();
        queue_create_info.queueCount = u32(queue_priorities.size());
    }

    VkPhysicalDeviceFeatures2 features = vk_struct();
    {
        features.features = required_features;
        features.pNext = &required_features_1_1;
        required_features_1_1.pNext = &required_features_1_2;
        required_features_1_2.pNext = &required_features_1_3;
        required_features_1_3.pNext = &required_features_1_4;
    }

    if(raytracing_enabled()) {
        required_features_ray_query.pNext = &required_features_accel;
        required_features_accel.pNext = &required_features_raytracing;
        required_features_raytracing.pNext =  &required_features_ray_position_fetch;
        required_features_ray_position_fetch.pNext = &required_features_shader_float_atomic;
        required_features_shader_float_atomic.pNext = features.pNext;

        features.pNext = &required_features_ray_query;
    }

    VkDeviceCreateInfo create_info = vk_struct();
    {
        create_info.pNext = &features;
        create_info.enabledExtensionCount = u32(device::enabled_extensions.size());
        create_info.ppEnabledExtensionNames = device::enabled_extensions.data();
        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;
    }

    if(instance_params().validation_layers) {
        const auto ext = validation_extensions();
        create_info.enabledLayerCount = u32(ext.size());
        create_info.ppEnabledLayerNames = ext.data();
    }

    {
        y_profile_zone("vkCreateDevice");
        vk_check(vkCreateDevice(physical_device().vk_physical_device(), &create_info, vk_allocation_callbacks(), &device::vk_device));
        volkLoadDevice(device::vk_device);
    }

    print_properties(device::device_properties);


    if(device::device_properties.has_raytracing && !raytracing_enabled()) {
        log_msg("Raytracing disabled", Log::Warning);
    }

    const VkQueue queue = create_queue(device::vk_device, main_queue_index, 0);
    device::queue.init(main_queue_index, queue);

    if(diagnostic_checkpoints) {
        log_msg("Vulkan diagnostic checkpoints enabled");
        device::diagnostic_checkpoints = std::make_unique<DiagnosticCheckpoints>();
        device::diagnostic_checkpoints->register_queue(*device::queue);
    }
}



void init_device(Instance& instance) {
    return init_device(instance, find_best_device(instance));
}

void init_device(Instance& instance, PhysicalDevice device) {
    y_always_assert(!device::instance, "Device already initialized");

    device::instance = &instance;
    device::physical_device = std::make_unique<PhysicalDevice>(device);
    device::device_properties = device::physical_device->device_properties();

    init_vk_device();

    init_vma();

    device::lifetime_manager.init();
    device::mesh_allocator.init();
    device::material_allocator.init();
    device::texture_library.init();
    device::layout_allocator.init();

    for(usize i = 0; i != device::samplers.size(); ++i) {
        device::samplers[i].init(create_sampler(SamplerType(i)));
    }

    y_profile_zone("loading resources");
    device::resources.init();
}

void destroy_device() {
    lifetime_manager().shutdown_collector_thread();
    wait_all_queues();

    device::resources.destroy();

    lifetime_manager().wait_cmd_buffers();

#ifdef Y_DEBUG
    device::destroying = true;
    y_defer(device::destroying = false);
#endif

    device::queue->clear_all_cmd_pools();

    for(auto& sampler : device::samplers) {
        sampler.destroy();
    }


    device::diagnostic_checkpoints = nullptr;

    device::layout_allocator.destroy();
    device::texture_library.destroy();
    device::material_allocator.destroy();
    device::mesh_allocator.destroy();
    device::lifetime_manager.destroy();

    device::queue.destroy();

    vmaDestroyAllocator(device::allocator);
    device::allocator = {};

    {
        y_profile_zone("vkDestroyDevice");
        vkDestroyDevice(device::vk_device, vk_allocation_callbacks());
        device::vk_device = {};
    }

    device::instance = nullptr;
}

bool device_initialized() {
    return device::instance != nullptr;
}

const InstanceParams& instance_params() {
    return device::instance->instance_params();
}




VkDevice vk_device() {
    return device::vk_device;
}

VkInstance vk_device_instance() {
    return device::instance->vk_instance();
}

VkPhysicalDevice vk_physical_device() {
    return physical_device().vk_physical_device();
}

const PhysicalDevice& physical_device() {
    return *device::physical_device;
}

CmdBufferRecorder create_disposable_cmd_buffer() {
    return device::queue->cmd_pool_for_thread().create_cmd_buffer();
}

ComputeCmdBufferRecorder create_disposable_compute_cmd_buffer() {
    return device::queue->cmd_pool_for_thread().create_compute_cmd_buffer();
}

TransferCmdBufferRecorder create_disposable_transfer_cmd_buffer() {
    return device::queue->cmd_pool_for_thread().create_transfer_cmd_buffer();
}

DescriptorLayoutAllocator& layout_allocator() {
    return *device::layout_allocator;
}

VmaAllocator device_allocator() {
    return device::allocator;
}

MeshAllocator& mesh_allocator() {
    return *device::mesh_allocator;
}

MaterialAllocator& material_allocator() {
    return *device::material_allocator;
}

TextureLibrary& texture_library() {
    return *device::texture_library;
}

CmdQueue& command_queue() {
    return *device::queue;
}

CmdQueue& loading_command_queue() {
    return *device::queue;
}

const DeviceResources& device_resources() {
    return *device::resources;
}

const DeviceProperties& device_properties() {
    return device::device_properties;
}

bool raytracing_enabled() {
    return device_properties().has_raytracing && instance_params().raytracing;
}

LifetimeManager& lifetime_manager() {
    return *device::lifetime_manager;
}

const VkAllocationCallbacks* vk_allocation_callbacks() {
#if 0
    static VkAllocationCallbacks callbacks = {
        nullptr,
        [](void*, usize size, usize alignment, VkSystemAllocationScope) -> void* {
            void* ptr = _aligned_malloc(size, alignment);;
            y_profile_alloc(ptr, size);
            return ptr;
        },
        [](void*, void* original, usize size, usize alignment, VkSystemAllocationScope) -> void* {
            y_profile_free(original);
            void* ptr = _aligned_realloc(original, size, alignment);
            y_profile_alloc(ptr, size);
            return ptr;
        },
        [](void*, void* ptr) {
            y_profile_free(ptr);
            _aligned_free(ptr);
        },
        nullptr, nullptr
    };
    return &callbacks;
#else
    return nullptr;
#endif
}

VkSampler vk_sampler(SamplerType type) {
    y_debug_assert(usize(type) < device::samplers.size());
    return device::samplers[usize(type)]->vk_sampler();
}

const DebugUtils* debug_utils() {
    return device::instance->debug_utils();
}

const DiagnosticCheckpoints* diagnostic_checkpoints() {
    return device::diagnostic_checkpoints.get();
}

bool is_extension_enabled(std::string_view ext_name) {
    return std::find_if(device::enabled_extensions.begin(), device::enabled_extensions.end(), [=](const char* name) { return ext_name == name; }) != device::enabled_extensions.end();
}

void wait_all_queues() {
    device::queue->wait();
}





#define YAVE_GENERATE_DESTROY_IMPL(T)                                                   \
    void destroy_graphic_resource(T&& t) {                                              \
        if(!t.is_null()) {                                                              \
            lifetime_manager().destroy_later(std::move(t));                             \
            y_debug_assert(t.is_null());                                                \
        }                                                                               \
    }
YAVE_GRAPHIC_HANDLE_TYPES(YAVE_GENERATE_DESTROY_IMPL)
#undef YAVE_GENERATE_DESTROY_IMPL

}

