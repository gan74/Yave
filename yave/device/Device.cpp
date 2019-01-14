/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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

#include <yave/device/Device.h>
#include <yave/graphics/commands/CmdBufferBase.h>

#include <mutex>

namespace yave {

static void check_features(const vk::PhysicalDeviceFeatures& features, const vk::PhysicalDeviceFeatures& required) {
	auto feats = reinterpret_cast<const vk::Bool32*>(&features);
	auto req = reinterpret_cast<const vk::Bool32*>(&required);
	for(usize i = 0; i != sizeof(features) / sizeof(vk::Bool32); ++i) {
		if(req[i] && !feats[i]) {
			y_fatal("Required Vulkan feature not supported");
		}
	}
}

static core::Vector<const char*> extensions(const DebugParams& debug) {
	auto exts = core::vector_with_capacity<const char*>(4);
	exts << VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	if(debug.debug_features_enabled()) {
		exts << DebugMarker::name();
	}

	return exts;
}

static vk::Device create_device(
		vk::PhysicalDevice physical,
		const core::ArrayView<QueueFamily>& queue_families,
		const DebugParams& debug) {

	auto queue_create_infos = core::vector_with_capacity<vk::DeviceQueueCreateInfo>(queue_families.size());

	auto prio_count = std::max_element(queue_families.begin(), queue_families.end(),
			[](const auto& a, const auto& b) { return a.count() < b.count(); })->count();
	core::Vector<float> priorities(prio_count, 1.0f);
	std::transform(queue_families.begin(), queue_families.end(), std::back_inserter(queue_create_infos), [&](const auto& q) {
		return vk::DeviceQueueCreateInfo()
				.setQueueFamilyIndex(q.index())
				.setPQueuePriorities(priorities.begin())
				.setQueueCount(q.count())
			;
		});

	auto required = vk::PhysicalDeviceFeatures();
	required.multiDrawIndirect = true;
	required.drawIndirectFirstInstance = true;
	required.fullDrawIndexUint32 = true;
	required.textureCompressionBC = true;
	required.shaderStorageImageExtendedFormats = true;
	required.shaderUniformBufferArrayDynamicIndexing = true;
	required.shaderSampledImageArrayDynamicIndexing = true;
	required.shaderStorageBufferArrayDynamicIndexing = true;
	required.shaderStorageImageArrayDynamicIndexing = true;

	if(debug.debug_features_enabled()) {
		required.robustBufferAccess = true;
	}

	check_features(physical.getFeatures(), required);

	auto exts = extensions(debug);

	return physical.createDevice(vk::DeviceCreateInfo()
			.setEnabledExtensionCount(u32(exts.size()))
			.setPpEnabledExtensionNames(exts.begin())
			.setEnabledLayerCount(u32(debug.device_layers().size()))
			.setPpEnabledLayerNames(debug.device_layers().begin())
			.setQueueCreateInfoCount(queue_create_infos.size())
			.setPQueueCreateInfos(queue_create_infos.begin())
			.setPEnabledFeatures(&required)
		);
}


Device::ScopedDevice::~ScopedDevice() {
	device.destroy();
}

Device::Device(Instance& instance) :
		_instance(instance),
		_physical(instance),
		_queue_families(QueueFamily::all(_physical)),
		_device{create_device(_physical.vk_physical_device(), _queue_families, _instance.debug_params())},
		_sampler(this),
		_allocator(this),
		_disposable_cmd_pool(this),
		_primary_cmd_pool(this),
		_descriptor_layout_pool(new DescriptorSetLayoutPool(this)) {

	if(_instance.debug_params().debug_features_enabled()) {
		_extensions.debug_marker = std::make_unique<DebugMarker>(_device.device);
	}

	for(const auto& family : _queue_families) {
		for(auto& queue : family.queues(this)) {
			_queues.push_back(std::move(queue));
		}
	}

	_default_resources = DefaultResources(this);
}

Device::~Device() {
}

const PhysicalDevice& Device::physical_device() const {
	return _physical;
}

const Instance &Device::instance() const {
	return _instance;
}

DeviceAllocator& Device::allocator() const {
	return _allocator;
}

const QueueFamily& Device::queue_family(vk::QueueFlags flags) const {
	for(const auto& q : _queue_families) {
		if((q.flags() & flags) == flags) {
			return q;
		}
	}
	return y_fatal("Unable to find queue.");
}

const Queue& Device::graphic_queue() const {
	return _queues.first();
}

Queue& Device::graphic_queue() {
	return _queues.first();
}

static usize generate_thread_id() {
	static concurrent::SpinLock lock;
	static usize id = 0;
	std::unique_lock _(lock);
	return ++id;
}

ThreadDevicePtr Device::thread_data() const {
	static thread_local usize thread_id = generate_thread_id();
	static thread_local std::pair<DevicePtr, ThreadDevicePtr> thread_cache;

	auto& cache = thread_cache;
	if(cache.first != this) {
		std::unique_lock _(_lock);
		while(_thread_local_datas.size() <= thread_id) {
			_thread_local_datas.emplace_back();
		}
		auto& data = _thread_local_datas[thread_id];
		if(!data) {
			data = std::make_unique<ThreadLocalDeviceData>(this);
		}
		cache = {this, data.get()};
		return data.get();
	}
	return cache.second;
}

const DefaultResources& Device::default_resources() const {
	return _default_resources;
}

const vk::PhysicalDeviceLimits& Device::vk_limits() const {
	return _physical.vk_properties().limits;
}

vk::Device Device::vk_device() const {
	return _device.device;
}

vk::Sampler Device::vk_sampler() const {
	return _sampler.vk_sampler();
}

CmdBuffer<CmdBufferUsage::Disposable> Device::create_disposable_cmd_buffer() const {
	return _disposable_cmd_pool.create_buffer();
}

CmdBuffer<CmdBufferUsage::Primary> Device::create_cmd_buffer() const {
	return _primary_cmd_pool.create_buffer();
}

const DebugMarker* Device::debug_marker() const {
	return _extensions.debug_marker.get();
}



}
