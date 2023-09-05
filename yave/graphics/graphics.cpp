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

#include "graphics.h"

#include <yave/graphics/device/deviceutils.h>
#include <yave/graphics/device/Instance.h>
#include <yave/graphics/device/DeviceProperties.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferPool.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/images/Sampler.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/descriptors/DescriptorSetAllocator.h>
#include <yave/graphics/memory/DeviceMemoryAllocator.h>
#include <yave/graphics/device/LifetimeManager.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>

#include <y/concurrent/Mutexed.h>
#include <y/core/ScratchPad.h>


namespace yave {
namespace device {

Instance* instance = nullptr;
std::unique_ptr<PhysicalDevice> physical_device;

DeviceProperties device_properties;

std::array<Uninitialized<Sampler>, 5> samplers;

Uninitialized<DeviceMemoryAllocator> allocator;
Uninitialized<LifetimeManager> lifetime_manager;
Uninitialized<DescriptorSetAllocator> descriptor_set_allocator;
Uninitialized<MeshAllocator> mesh_allocator;
Uninitialized<MaterialAllocator> material_allocator;
Uninitialized<TextureLibrary> texture_library;
Uninitialized<DeviceResources> resources;

VkDevice vk_device;

core::FixedArray<std::unique_ptr<CmdQueue>> queues;
std::atomic<u64> next_loading_queue_index = 0;

std::atomic<u64> active_pools = 0;

#ifdef Y_DEBUG
std::atomic<bool> destroying = false;
#endif

struct {
    VkSemaphore semaphore = {};
    std::atomic<u64> fence_value = 0;
    std::atomic<u64> last_polled = 0;
} timeline;


struct ThreadDeviceData : NonMovable {
    ThreadDeviceData() {
        ++active_pools;
    }

    ~ThreadDeviceData() {
        --active_pools;
    }

    CmdBufferPool cmd_pool;
};

struct ThreadDeviceDataStorage {
    std::unique_ptr<ThreadDeviceData> storage;
    ThreadDeviceData** thread_local_ptr = nullptr;
};

concurrent::Mutexed<core::Vector<ThreadDeviceDataStorage>> thread_device_datas;

}

static device::ThreadDeviceData* thread_device_data() {
    static thread_local device::ThreadDeviceData* this_thread_data = nullptr;

    static thread_local y_defer({
        if(auto* data = this_thread_data) {
            y_debug_assert(device_initialized());
            device::thread_device_datas.locked([&](auto& datas) {
                const auto it = std::find_if(datas.begin(), datas.end(), [&](const auto& p) { return p.storage.get() == data; });
                y_always_assert(it != datas.end(), "ThreadDeviceData not found.");
                y_debug_assert(*it->thread_local_ptr == data);
                *it->thread_local_ptr = nullptr;
                datas.erase_unordered(it);
                y_always_assert(device::active_pools == datas.size(), "Invalid number of pools");
            });
        }
    });

    y_debug_assert(device_initialized());

    if(auto* data = this_thread_data) {
        return data;
    }

    y_debug_assert(!device::destroying);

    auto data = std::make_unique<device::ThreadDeviceData>();
    this_thread_data = data.get();

    device::thread_device_datas.locked([&](auto&& datas) {
        datas.emplace_back(std::move(data), &this_thread_data);
    });

    return this_thread_data;
}



static void init_timeline() {
    VkSemaphoreTypeCreateInfo type_create_info = vk_struct();
    type_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;

    VkSemaphoreCreateInfo create_info = vk_struct();
    create_info.pNext = &type_create_info;

    vk_check(vkCreateSemaphore(device::vk_device, &create_info, vk_allocation_callbacks(), &device::timeline.semaphore));

    VkSemaphoreSignalInfo signal_info = vk_struct();
    signal_info.semaphore = device::timeline.semaphore;
    signal_info.value = create_timeline_fence().value();

    vk_check(vkSignalSemaphore(device::vk_device, &signal_info));
}

static void init_vk_device() {
    y_profile();

    const DebugParams& debug = device::instance->debug_params();

    const core::Vector<VkQueueFamilyProperties> queue_families = enumerate_family_properties(vk_physical_device());

    const VkQueueFlags graphic_queue_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    const u32 main_queue_index = queue_family_index(queue_families, graphic_queue_flags);

    const VkQueueFamilyProperties main_queue_family_properties = queue_families[main_queue_index];
    const usize queue_count = std::min(main_queue_family_properties.queueCount, 5u); // 1 main + 4 loading

    auto extensions = core::Vector<const char*>::with_capacity(4);
    extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };


    const auto required_features = required_device_features();
    auto required_features_1_1 = required_device_features_1_1();
    auto required_features_1_2 = required_device_features_1_2();
    auto required_features_1_3 = required_device_features_1_3();

    if(physical_device().vk_properties_1_3().maxInlineUniformBlockSize > 0) {
        required_features_1_3.inlineUniformBlock = true;
    }

    y_always_assert(has_required_features(physical_device()), "Device doesn't support required features");
    y_always_assert(has_required_properties(physical_device()), "Device doesn't support required properties");

    print_physical_properties(physical_device().vk_properties());
    print_enabled_extensions(extensions);


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
    }

    VkDeviceCreateInfo create_info = vk_struct();
    {
        create_info.pNext = &features;
        create_info.enabledExtensionCount = u32(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();
        create_info.enabledLayerCount = u32(debug.device_layers().size());
        create_info.ppEnabledLayerNames = debug.device_layers().data();
        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;
    }

    {
        y_profile_zone("vkCreateDevice");
        vk_check(vkCreateDevice(physical_device().vk_physical_device(), &create_info, vk_allocation_callbacks(), &device::vk_device));
        volkLoadDevice(device::vk_device);
    }

    print_properties(device::device_properties);

    device::queues = core::FixedArray<std::unique_ptr<CmdQueue>>(queue_count);
    for(u32 i = 0; i != device::queues.size(); ++i) {
        device::queues[i] = std::make_unique<CmdQueue>(main_queue_index, create_queue(device::vk_device, main_queue_index, i));
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
    init_timeline();

    device::lifetime_manager.init();
    device::allocator.init(device_properties());
    device::descriptor_set_allocator.init();
    device::mesh_allocator.init();
    device::material_allocator.init();
    device::texture_library.init();

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

    y_always_assert(device::active_pools == 1, "Not all pools have been destroyed");
    device::thread_device_datas.locked([](auto&& datas) {
        for(auto& p : datas) {
            *p.thread_local_ptr = nullptr;
        }
        datas.clear();
    });
    y_always_assert(device::active_pools == 0, "Not all pools have been destroyed");

    for(auto& sampler : device::samplers) {
        sampler.destroy();
    }

    device::texture_library.destroy();
    device::material_allocator.destroy();
    device::mesh_allocator.destroy();
    device::descriptor_set_allocator.destroy();
    device::lifetime_manager.destroy();
    device::allocator.destroy();

    vkDestroySemaphore(device::vk_device, device::timeline.semaphore, vk_allocation_callbacks());

    device::queues.clear();

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




const DebugParams& debug_params() {
    return device::instance->debug_params();
}

TimelineFence create_timeline_fence() {
    return TimelineFence(++device::timeline.fence_value);
}

bool poll_fence(const TimelineFence& fence) {
    if(device::timeline.last_polled >= fence.value()) {
        return true;
    }

    u64 value = 0;
    vk_check(vkGetSemaphoreCounterValue(device::vk_device, device::timeline.semaphore, &value));
    update_maximum(device::timeline.last_polled, value);

    return device::timeline.last_polled >= fence.value();

}

void wait_for_fence(const TimelineFence& fence) {
    if(device::timeline.last_polled >= fence.value()) {
        return;
    }

    y_profile();

    const u64 fence_value = fence.value();
    VkSemaphoreWaitInfo wait_info = vk_struct();
    {
        wait_info.pSemaphores = &device::timeline.semaphore;
        wait_info.pValues = &fence_value;
        wait_info.semaphoreCount = 1;
    }

    static constexpr u64 timeout_ns = 10'000'000'000llu; // 10s
    if(vkWaitSemaphores(device::vk_device, &wait_info, timeout_ns) != VK_SUCCESS) {
        y_fatal("Semaphore timeout expired");
    }

    update_maximum(device::timeline.last_polled, fence_value);
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

VkSemaphore vk_timeline_semaphore() {
    return device::timeline.semaphore;
}

const PhysicalDevice& physical_device() {
    return *device::physical_device;
}

CmdBufferRecorder create_disposable_cmd_buffer() {
    return thread_device_data()->cmd_pool.create_cmd_buffer();
}

ComputeCmdBufferRecorder create_disposable_compute_cmd_buffer() {
    return thread_device_data()->cmd_pool.create_compute_cmd_buffer();
}

TransferCmdBufferRecorder create_disposable_transfer_cmd_buffer() {
    return thread_device_data()->cmd_pool.create_transfer_cmd_buffer();
}

DeviceMemoryAllocator& device_allocator() {
    return device::allocator.get();
}

DescriptorSetAllocator& descriptor_set_allocator() {
    return device::descriptor_set_allocator.get();
}

MeshAllocator& mesh_allocator() {
    return device::mesh_allocator.get();
}

MaterialAllocator& material_allocator() {
    return device::material_allocator.get();
}

TextureLibrary& texture_library() {
    return device::texture_library.get();
}

const CmdQueue& command_queue() {
    return *device::queues[0];
}

const CmdQueue& loading_command_queue() {
    if(const usize loading_queue_count = device::queues.size() - 1) {
        device::queues[++device::next_loading_queue_index % loading_queue_count];
    }
    return *device::queues[0];
}

const DeviceResources& device_resources() {
    return device::resources.get();
}

const DeviceProperties& device_properties() {
    return device::device_properties;
}

LifetimeManager& lifetime_manager() {
    return device::lifetime_manager.get();
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
    return device::samplers[usize(type)].get().vk_sampler();
}

const DebugUtils* debug_utils() {
    return device::instance->debug_utils();
}

void wait_all_queues() {
    Y_TODO(This is wrong)
    for(auto& queue : device::queues) {
        queue->wait();
    }
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

