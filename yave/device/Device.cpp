/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#include <yave/commands/CmdBuffer.h>

#include <mutex>

namespace yave {

static void check_features(const vk::PhysicalDeviceFeatures& features, const vk::PhysicalDeviceFeatures& required) {
	auto feats = reinterpret_cast<const vk::Bool32*>(&features);
	auto req = reinterpret_cast<const vk::Bool32*>(&required);
	for(usize i = 0; i != sizeof(features) / sizeof(vk::Bool32); ++i) {
		if(req[i] && !feats[i]) {
			fatal("Required Vulkan feature not supported");
		}
	}
}

static core::Vector<QueueFamily> compute_queue_families(vk::PhysicalDevice physical) {
	core::Vector<QueueFamily> queue_families;

	auto families = physical.getQueueFamilyProperties();
	for(u32 i = 0; i != families.size(); ++i) {
		queue_families << QueueFamily(i, families[i]);
	}
	return queue_families;
}

static vk::Device create_device(
		vk::PhysicalDevice physical,
		const core::Vector<QueueFamily>& queue_families,
		const core::Vector<const char*>& extentions,
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

	if(debug.is_debug_callback_enabled()) {
		required.robustBufferAccess = true;
	}

	check_features(physical.getFeatures(), required);

	return physical.createDevice(vk::DeviceCreateInfo()
			.setEnabledExtensionCount(u32(extentions.size()))
			.setPpEnabledExtensionNames(extentions.begin())
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
		_queue_families(compute_queue_families(_physical.vk_physical_device())),
		_device{create_device(_physical.vk_physical_device(), _queue_families, {VK_KHR_SWAPCHAIN_EXTENSION_NAME}, _instance.debug_params())},
		_sampler(this),
		_allocator(this),
		_secondary_cmd_pool(this),
		_disposable_cmd_pool(this),
		_primary_cmd_pool(this),
		_descriptor_layout_pool(new DescriptorSetLayoutPool(this)) {

	for(const auto& family : _queue_families) {
		auto q = family.fetch_queues(this);
		_queues.push_back(q.begin(), q.end());
	}
}

Device::~Device() {
	for(auto q : _queues) {
		q.waitIdle();
	}
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
	return fatal("Unable to find queue.");
}

ThreadDevicePtr Device::thread_data() const {
	std::unique_lock lock(_lock);
	auto thread_id = std::this_thread::get_id();
	if(auto it = _thread_local_datas.find(thread_id); it != _thread_local_datas.end()) {
		return it->second.as_ptr();
	}
	return (_thread_local_datas[thread_id] = new ThreadLocalDeviceData(this)).as_ptr();
}

const vk::PhysicalDeviceLimits& Device::vk_limits() const {
	return _physical.vk_properties().limits;
}

vk::Device Device::vk_device() const {
	return _device.device;
}

vk::Queue Device::vk_queue(vk::QueueFlags) const {
#warning change Device::vk_queue
	return _queues.first();
}

vk::Sampler Device::vk_sampler() const {
	return _sampler.vk_sampler();
}

CmdBuffer<CmdBufferUsage::Disposable> Device::create_disposable_cmd_buffer() const {
	return _disposable_cmd_pool.create_buffer();
}

CmdBuffer<CmdBufferUsage::Secondary> Device::create_secondary_cmd_buffer() const {
	return _secondary_cmd_pool.create_buffer();
}

CmdBuffer<CmdBufferUsage::Primary> Device::create_cmd_buffer() const {
	return _primary_cmd_pool.create_buffer();
}


}
