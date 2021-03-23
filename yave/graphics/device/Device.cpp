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

#include "Device.h"
#include "PhysicalDevice.h"

#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <y/concurrent/concurrent.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <mutex>

namespace yave {

Device* Device::_main_device = nullptr;

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


static bool try_enable_extension(core::Vector<const char*>& exts, const char* name, const PhysicalDevice& device) {
    if(device.is_extension_supported(name)) {
        exts << name;
        return true;
    }
    log_msg(fmt("% not supported", name), Log::Warning);
    return false;
}

static std::array<Sampler, 5> create_samplers() {
    std::array<Sampler, 5> samplers = {
        SamplerType::LinearRepeat,
        SamplerType::LinearClamp,
        SamplerType::PointRepeat,
        SamplerType::PointClamp,
        SamplerType::Shadow,
    };
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

static VkQueue create_queue(u32 family_index, u32 index) {
    VkQueue q = {};
    vkGetDeviceQueue(vk_device(), family_index, index, &q);
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



static const VkQueueFlags graphic_queue_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;

Device::Device(Instance& instance) : Device(instance, find_best_device(instance.physical_devices())) {
}


Device::Device(Instance& instance, PhysicalDevice device) :
        _instance(instance),
        _physical(std::make_unique<PhysicalDevice>(device)),
        _properties(_physical->device_properties()) {

    y_profile();

    const DebugParams& debug = _instance.debug_params();

    const core::Vector<VkQueueFamilyProperties> queue_families = enumerate_family_properties(vk_physical_device());
    _main_queue_index = queue_family_index(queue_families, graphic_queue_flags);

    auto extensions = core::vector_with_capacity<const char*>(4);
    extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    try_enable_extension(extensions, VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME, physical_device());

    const auto required_features = Device::required_device_features();
    auto required_features_1_1 = Device::required_device_features_1_1();
    auto required_features_1_2 = Device::required_device_features_1_2();

    y_always_assert(Device::has_required_features(physical_device()), "Device doesn't support required features");
    y_always_assert(Device::has_required_properties(physical_device()), "Device doesn't support required properties");

    print_physical_properties(physical_device().vk_properties());
    print_enabled_extensions(extensions);


    const std::array queue_priorityies = {1.0f, 0.0f};

    VkDeviceQueueCreateInfo queue_create_info = vk_struct();
    {
        queue_create_info.queueFamilyIndex = _main_queue_index;
        queue_create_info.pQueuePriorities = queue_priorityies.data();
        queue_create_info.queueCount = u32(queue_priorityies.size());
    }

    VkPhysicalDeviceFeatures2 features = vk_struct();
    {
        features.features = required_features;
        features.pNext = &required_features_1_1;
        required_features_1_1.pNext = &required_features_1_2;
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
        vk_check(vkCreateDevice(physical_device().vk_physical_device(), &create_info, nullptr, &_device));
    }

    print_properties(_properties);

    y_always_assert(_main_device == nullptr, "Device already exists.");
    _main_device = this;

    _graphic_queue = Queue(_main_queue_index, create_queue(_main_queue_index, 0));
    _loading_queue = Queue(_main_queue_index, create_queue(_main_queue_index, 1));

    for(usize i = 0; i != _samplers.size(); ++i) {
        _samplers[i].init(SamplerType(i));
    }

    _lifetime_manager.init();
    _allocator.init();
    _descriptor_set_allocator.init();

    {
        y_profile_zone("loading resources");
        _resources.init();
    }
}

Device::~Device() {
    y_profile();

    y_always_assert(_main_device == this, "Device does not exist.");

    wait_all_queues();

    {
        y_profile_zone("clearing resources");
        _resources.destroy();
    }

    {
        y_profile_zone("flushing command pools");
        CmdBufferRecorder(CmdBufferPool().create_buffer()).submit<SyncPolicy::Sync>();
        lifetime_manager().wait_cmd_buffers();
    }

    {
        y_profile_zone("collecting thread devices");
        const auto lock = y_profile_unique_lock(_devices_lock);
        for(const auto& p : _thread_devices) {
            *p.second = nullptr;
        }
        _thread_devices.clear();
    }

    for(auto& sampler : _samplers) {
        sampler.destroy();
    }

    _descriptor_set_allocator.destroy();
    _lifetime_manager.destroy();
    _allocator.destroy();

    _graphic_queue = {};
    _loading_queue = {};

    {
        y_profile_zone("vkDestroyDevice");
        vkDestroyDevice(_device, nullptr);
        _device = {};
    }

    _main_device = nullptr;
}


void Device::create_thread_device(ThreadDevicePtr* dptr) const {
    y_debug_assert(dptr && *dptr == nullptr);

    auto device = std::make_unique<ThreadLocalDevice>(this);
    *dptr = device.get();

    const auto lock = y_profile_unique_lock(_devices_lock);
    _thread_devices.emplace_back(std::move(device), dptr);
}

void Device::destroy_thread_device(ThreadDevicePtr device) const {
    y_debug_assert(device);

    const auto lock = y_profile_unique_lock(_devices_lock);
    const auto it = std::find_if(_thread_devices.begin(), _thread_devices.end(), [&](const auto& p) { return p.first.get() == device; });

    y_always_assert(it != _thread_devices.end(), "ThreadLocalDevice not found.");
    y_debug_assert(*it->second == device);

    *it->second = nullptr;
    _thread_devices.erase_unordered(it);
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
        required.robustBufferAccess = true;
        required.independentBlend = true;
    }

    return required;
}

VkPhysicalDeviceVulkan11Features Device::required_device_features_1_1() {
    VkPhysicalDeviceVulkan11Features required = vk_struct();

    {
        // We don't actually need anything here...
    }

    return required;
}

VkPhysicalDeviceVulkan12Features Device::required_device_features_1_2() {
    VkPhysicalDeviceVulkan12Features required = vk_struct();

    {
        //required.timelineSemaphore = true;
    }

    return required;
}

bool Device::has_required_features(const PhysicalDevice& physical) {
    if(!physical.support_features(Device::required_device_features())) {
        return false;
    }

    if(!physical.support_features(Device::required_device_features_1_1())) {
        return false;
    }

    if(!physical.support_features(Device::required_device_features_1_2())) {
        return false;
    }

    return true;
}

bool Device::has_required_properties(const PhysicalDevice &physical) {
    const auto& p11 = physical.vk_properties_1_1();

    bool ok = true;

    ok &= !!p11.subgroupQuadOperationsInAllStages;

    return ok;
}

DevicePtr Device::main_device() {
    y_debug_assert(_main_device);
    return _main_device;
}


const PhysicalDevice& Device::physical_device() const {
    return *_physical;
}

const Instance &Device::instance() const {
    return _instance;
}

DeviceMemoryAllocator& Device::allocator() const {
    return _allocator.get();
}

DescriptorSetAllocator& Device::descriptor_set_allocator() const {
    return _descriptor_set_allocator.get();
}

const Queue& Device::graphic_queue() const {
    return _graphic_queue;
}

const Queue& Device::loading_queue() const {
    return _loading_queue;
}

void Device::wait_all_queues() const {
    Y_TODO(Remove and use lock_all_queues_and_wait everywhere)
    lock_all_queues_and_wait();
}

concurrent::VectorLock<std::mutex> Device::lock_all_queues_and_wait() const {
    y_profile();

    auto lock = concurrent::VectorLock<std::mutex>({
        _graphic_queue.lock(),
        _loading_queue.lock()
    });

    vk_check(vkDeviceWaitIdle(vk_device()));

    return lock;
}

ThreadDevicePtr Device::thread_device() const {
    static thread_local ThreadDevicePtr thread_device = nullptr;
    static thread_local y_defer({
        if(thread_device) {
            thread_device->parent()->destroy_thread_device(thread_device);
        }
    });

    if(!thread_device) {
        create_thread_device(&thread_device);
    }
    return thread_device;
}

const DeviceResources& Device::device_resources() const {
    return _resources.get();
}

DeviceResources& Device::device_resources() {
    return _resources.get();
}

LifetimeManager& Device::lifetime_manager() const {
    return _lifetime_manager.get();
}

const DeviceProperties& Device::device_properties() const {
    return _properties;
}

VkDevice Device::vk_device() const {
    return _device;
}

const VkAllocationCallbacks* Device::vk_allocation_callbacks() const {
    return nullptr;
}

VkPhysicalDevice Device::vk_physical_device() const {
    return _physical->vk_physical_device();
}

VkSampler Device::vk_sampler(SamplerType type) const {
    y_debug_assert(usize(type) < _samplers.size());
    return _samplers[usize(type)].get().vk_sampler();
}

const DebugUtils* Device::debug_utils() const {
    return _instance.debug_utils();
}

const RayTracing* Device::ray_tracing() const {
    return nullptr;
}

}

