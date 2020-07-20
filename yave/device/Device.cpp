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

#include <yave/device/extentions/RayTracing.h>

#include <y/concurrent/concurrent.h>

#include <yave/graphics/commands/CmdBufferBase.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <mutex>

//#define YAVE_NV_RAY_TRACING

namespace yave {

static void check_features(const VkPhysicalDeviceFeatures& features, const VkPhysicalDeviceFeatures& required) {
	const auto feats = reinterpret_cast<const VkBool32*>(&features);
	const auto req = reinterpret_cast<const VkBool32*>(&required);
	for(usize i = 0; i != sizeof(features) / sizeof(VkBool32); ++i) {
		if(req[i] && !feats[i]) {
			y_fatal("Required Vulkan feature not supported");
		}
	}
}

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

static core::Vector<Queue> create_queues(DevicePtr dptr, core::Span<QueueFamily> families) {
	core::Vector<Queue> queues;
	for(const auto& family : families) {
		for(auto& queue : family.queues(dptr)) {
			queues.push_back(std::move(queue));
		}
	}
	return queues;
}


static std::array<Sampler, 2> create_samplers(DevicePtr dptr) {
	std::array<Sampler, 2> samplers;
	for(usize i = 0; i != samplers.size(); ++i) {
		samplers[i] = Sampler(dptr, Sampler::Type(i));
	}
	return samplers;
}

static VkDevice create_device(
		VkPhysicalDevice physical,
		const core::Span<QueueFamily> queue_families,
		const DebugParams& debug) {

	y_profile();

	auto queue_create_infos = core::vector_with_capacity<VkDeviceQueueCreateInfo>(queue_families.size());

	const auto prio_count = std::max_element(queue_families.begin(), queue_families.end(),
			[](const auto& a, const auto& b) { return a.count() < b.count(); })->count();
	const core::Vector<float> priorities(prio_count, 1.0f);
	std::transform(queue_families.begin(), queue_families.end(), std::back_inserter(queue_create_infos), [&](const auto& q) {
		VkDeviceQueueCreateInfo create_info = vk_struct();
		{
			create_info.queueFamilyIndex = q.index();
			create_info.pQueuePriorities = priorities.data();
			create_info.queueCount = q.count();
		}
		return create_info;
	});


	auto extensions = core::vector_with_capacity<const char*>(4);
	extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		// Vulkan 1.1
		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
	};

	try_enable_extension(extensions, VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME, physical);

#ifdef YAVE_NV_RAY_TRACING
	try_enable_extension(extensions, RayTracing::extension_name(), physical);
#else
	log_msg(fmt("% disabled", RayTracing::extension_name()), Log::Warning);
#endif

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

	{
		VkPhysicalDeviceFeatures supported = {};
		vkGetPhysicalDeviceFeatures(physical, &supported);
		check_features(supported, required);
	}

	VkDeviceCreateInfo create_info = vk_struct();
	{
		create_info.enabledExtensionCount = extensions.size();
		create_info.ppEnabledExtensionNames = extensions.data();
		create_info.enabledLayerCount = debug.device_layers().size();
		create_info.ppEnabledLayerNames = debug.device_layers().data();
		create_info.queueCreateInfoCount = queue_create_infos.size();
		create_info.pQueueCreateInfos = queue_create_infos.data();
		create_info.pEnabledFeatures = &required;
	}

	VkDevice device = {};
	vk_check(vkCreateDevice(physical, &create_info, nullptr, &device));
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


Device::Device(Instance& instance) :
		_instance(instance),
		_physical(instance),
		_queue_families(QueueFamily::all(_physical)),
		_device{create_device(_physical.vk_physical_device(), _queue_families, _instance.debug_params())},
		_properties(create_properties(_physical)),
		_allocator(this),
		_lifetime_manager(this),
		_queues(create_queues(this, _queue_families)),
		_samplers(create_samplers(this)),
		_descriptor_set_allocator(this) {

#ifdef YAVE_NV_RAY_TRACING
	if(is_extension_supported(RayTracing::extension_name(), _physical.vk_physical_device())) {
		_extensions.raytracing = std::make_unique<RayTracing>(this);
	}
#endif

	_resources.init(this);

	print_properties(_properties);
}

Device::~Device() {
	_resources = DeviceResources();
	_lifetime_manager.stop_async_collection();

	{
		Y_TODO(Why do we need this?)
		CmdBufferPool<CmdBufferUsage::Disposable> pool(this);
		CmdBufferRecorder rec = pool.create_buffer();
		graphic_queue().submit<SyncSubmit>(RecordedCmdBuffer(std::move(rec)));
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

const QueueFamily& Device::queue_family(VkQueueFlags flags) const {
	for(const auto& q : _queue_families) {
		if((q.flags() & flags) == flags) {
			return q;
		}
	}
	y_fatal("Unable to find queue.");
}

const Queue& Device::graphic_queue() const {
	return _queues.first();
}

Queue& Device::graphic_queue() {
	return _queues.first();
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

VkSampler Device::vk_sampler(Sampler::Type type) const {
	y_debug_assert(usize(type) < _samplers.size());
	return _samplers[usize(type)].vk_sampler();
}

CmdBuffer<CmdBufferUsage::Disposable> Device::create_disposable_cmd_buffer() const {
	return thread_device()->create_disposable_cmd_buffer();
}

const DebugUtils* Device::debug_utils() const {
	return _instance.debug_utils();
}

const RayTracing* Device::ray_tracing() const {
	return _extensions.raytracing.get();
}

}
