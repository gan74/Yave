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

#include "Device.h"
#include "PhysicalDevice.h"

#include <yave/graphics/device/extentions/RayTracing.h>

#include <y/concurrent/concurrent.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <mutex>

//#define YAVE_NV_RAY_TRACING

namespace yave {

static float device_score(const PhysicalDevice& device) {
    if(!device.support_features(Device::required_device_features())) {
        return -std::numeric_limits<float>::max();
    }

    const usize heap_size = device.total_device_memory() / (1024 * 1024);
    const float heap_score = float(heap_size) / float(heap_size + 8 * 1024);
    const float type_score = device.is_discrete() ? 1.0f : 0.0f;
    return heap_score + type_score;
}

static const PhysicalDevice& find_best_device(core::Span<PhysicalDevice> devices) {
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



static const VkQueueFlags graphic_queue_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;

static bool is_extension_supported(const char* name, VkPhysicalDevice device) {
    core::Vector<VkExtensionProperties> supported_extensions;
    {
        u32 count = 0;
        vk_check(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr));
        supported_extensions = core::Vector<VkExtensionProperties>(count, VkExtensionProperties{});
        vk_check(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, supported_extensions.data()));
    }

    const std::string_view name_view = name;
    for(const VkExtensionProperties& ext : supported_extensions) {
        if(name_view == ext.extensionName) {
            return true;
        }
    }
    return false;
}

static bool try_enable_extension(core::Vector<const char*>& exts, const char* name, VkPhysicalDevice physical) {
    if(is_extension_supported(name, physical)) {
        exts << name;
        return true;
    }
    log_msg(fmt("% not supported", name), Log::Warning);
    return false;
}

static std::array<Sampler, 2> create_samplers(DevicePtr dptr) {
    std::array<Sampler, 2> samplers;
    for(usize i = 0; i != samplers.size(); ++i) {
        samplers[i] = Sampler(dptr, SamplerType(i));
    }
    return samplers;
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

static u32 queue_family_index(VkPhysicalDevice device, VkQueueFlags flags) {
    const core::Vector<VkQueueFamilyProperties> families = enumerate_family_properties(device);
    return queue_family_index(families, flags);
}

static VkQueue create_queue(DevicePtr dptr, u32 index) {
    VkQueue q = {};
    vkGetDeviceQueue(vk_device(dptr), index, 0, &q);
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


VkPhysicalDeviceFeatures Device::required_device_features() {
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
    }

    return required;
}


static VkDevice create_device(
        const PhysicalDevice& physical,
        u32 graphic_queue_index,
        const DebugParams& debug) {

    y_profile();

    Y_TODO(Use Vulkan 1.1 ext stuff)
    auto extensions = core::vector_with_capacity<const char*>(4);
    extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        // Vulkan 1.1
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    };

    try_enable_extension(extensions, VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME, physical.vk_physical_device());

#ifdef YAVE_NV_RAY_TRACING
    try_enable_extension(extensions, RayTracing::extension_name(), physical);
#else
    log_msg(fmt("% disabled", RayTracing::extension_name()), Log::Warning);
#endif

    const VkPhysicalDeviceFeatures required_features = Device::required_device_features();
    y_always_assert(physical.support_features(required_features), "Device doesn't support required features");

    const float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = vk_struct();
    {
        queue_create_info.queueFamilyIndex = graphic_queue_index;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_info.queueCount = 1;
    }

    VkDeviceCreateInfo create_info = vk_struct();
    {
        create_info.enabledExtensionCount = extensions.size();
        create_info.ppEnabledExtensionNames = extensions.data();
        create_info.enabledLayerCount = debug.device_layers().size();
        create_info.ppEnabledLayerNames = debug.device_layers().data();
        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;
        create_info.pEnabledFeatures = &required_features;
    }

    print_physical_properties(physical.vk_properties());

    VkDevice device = {};
    vk_check(vkCreateDevice(physical.vk_physical_device(), &create_info, nullptr, &device));
    return device;
}


static DeviceProperties create_properties(const PhysicalDevice& device) {
    const VkPhysicalDeviceLimits& limits = device.vk_properties().limits;

    DeviceProperties properties = {};

    properties.non_coherent_atom_size = limits.nonCoherentAtomSize;
    properties.max_uniform_buffer_size = limits.maxUniformBufferRange;
    properties.uniform_buffer_alignment = limits.minUniformBufferOffsetAlignment;
    properties.storage_buffer_alignment = limits.minStorageBufferOffsetAlignment;

    properties.max_memory_allocations = limits.maxMemoryAllocationCount;

    properties.max_inline_uniform_size = 0;
    if(is_extension_supported(VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME, device.vk_physical_device())) {
        properties.max_inline_uniform_size = device.vk_uniform_block_properties().maxInlineUniformBlockSize;
    }


    return properties;
}

static void print_properties(const DeviceProperties& properties) {
    log_msg(fmt("max_memory_allocations = %", properties.max_memory_allocations), Log::Debug);
    log_msg(fmt("max_inline_uniform_size = %", properties.max_inline_uniform_size), Log::Debug);
    log_msg(fmt("max_uniform_buffer_size = %", properties.max_uniform_buffer_size), Log::Debug);
}





Device::ScopedDevice::~ScopedDevice() {
    vkDestroyDevice(device, nullptr);
}


Device::Device(Instance& instance) : Device(instance, find_best_device(instance.physical_devices())) {
}

Device::Device(Instance& instance, PhysicalDevice device) :
        _instance(instance),
        _physical(device),
        _graphic_queue_index(queue_family_index(_physical.vk_physical_device(), graphic_queue_flags)),
        _device{create_device(_physical, _graphic_queue_index, _instance.debug_params())},
        _properties(create_properties(_physical)),
        _allocator(this),
        _lifetime_manager(this),
        _graphic_queue(this, _graphic_queue_index, create_queue(this, _graphic_queue_index)),
        _samplers(create_samplers(this)),
        _descriptor_set_allocator(this) {

#ifdef YAVE_NV_RAY_TRACING
    if(is_extension_supported(RayTracing::extension_name(), _physical.vk_physical_device())) {
        _extensions.raytracing = std::make_unique<RayTracing>(this);
    }
#endif

    print_properties(_properties);

    _resources.init(this);

}

Device::~Device() {
    _resources = DeviceResources();
    _lifetime_manager.stop_async_collection();

    {
        Y_TODO(Why do we need this?)
        CmdBufferPool pool(this);
        CmdBufferRecorder rec = pool.create_buffer();
        std::move(rec).submit<SyncPolicy::Sync>();
    }

    wait_all_queues();
    _thread_devices.clear();
    wait_all_queues();

    _lifetime_manager.collect();
}

const PhysicalDevice& Device::physical_device() const {
    return _physical;
}

const Instance &Device::instance() const {
    return _instance;
}

DeviceMemoryAllocator& Device::allocator() const {
    return _allocator;
}

DescriptorSetAllocator& Device::descriptor_set_allocator() const {
    return _descriptor_set_allocator;
}

const Queue& Device::graphic_queue() const {
    return _graphic_queue;
}

Queue& Device::graphic_queue() {
    return _graphic_queue;
}

void Device::wait_all_queues() const {
    y_profile();
    vk_check(vkDeviceWaitIdle(vk_device()));
}

ThreadDevicePtr Device::thread_device() const {
    static thread_local usize thread_id = concurrent::thread_id();
    static thread_local std::pair<DevicePtr, ThreadDevicePtr> thread_cache;

    auto& cache = thread_cache;
    if(cache.first != this) {
        const auto lock = y_profile_unique_lock(_lock);
        while(_thread_devices.size() <= thread_id) {
            _thread_devices.emplace_back();
        }
        auto& data = _thread_devices[thread_id];
        if(!data) {
            data = std::make_unique<ThreadLocalDevice>(this);
            if(_thread_devices.size() > 64) {
                log_msg("64 ThreadLocalDevice have been created.", Log::Warning);
            }
        }
        cache = {this, data.get()};
        return data.get();
    }
    return cache.second;
}

const DeviceResources& Device::device_resources() const {
    y_debug_assert(_resources.device() == this);
    return _resources;
}

DeviceResources& Device::device_resources() {
    y_debug_assert(_resources.device() == this);
    return _resources;
}

LifetimeManager& Device::lifetime_manager() const {
    return _lifetime_manager;
}

const DeviceProperties& Device::device_properties() const {
    return _properties;
}

VkDevice Device::vk_device() const {
    return _device.device;
}

const VkAllocationCallbacks* Device::vk_allocation_callbacks() const {
    return nullptr;
}

VkPhysicalDevice Device::vk_physical_device() const {
    return _physical.vk_physical_device();
}

VkSampler Device::vk_sampler(SamplerType type) const {
    y_debug_assert(usize(type) < _samplers.size());
    return _samplers[usize(type)].vk_sampler();
}

CmdBuffer Device::create_disposable_cmd_buffer() const {
    return thread_device()->create_disposable_cmd_buffer();
}

const DebugUtils* Device::debug_utils() const {
    return _instance.debug_utils();
}

const RayTracing* Device::ray_tracing() const {
    return _extensions.raytracing.get();
}

}

