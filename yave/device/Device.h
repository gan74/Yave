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
#ifndef YAVE_DEVICE_DEVICE_H
#define YAVE_DEVICE_DEVICE_H

#include <yave/yave.h>

#include "DeviceProperties.h"
#include "PhysicalDevice.h"
#include "ThreadLocalDevice.h"
#include "DeviceResources.h"
#include "LifetimeManager.h"

#include <yave/graphics/descriptors/DescriptorSetAllocator.h>

#include <yave/graphics/images/Sampler.h>
#include <yave/graphics/queues/QueueFamily.h>
#include <yave/graphics/memory/DeviceMemoryAllocator.h>

#include <thread>

namespace yave {

class Device : NonMovable {

	struct ScopedDevice {
		~ScopedDevice();
		const VkDevice device;
	};

	public:
		explicit Device(Instance& instance);
		~Device();

		const PhysicalDevice& physical_device() const;
		const Instance& instance() const;

		DeviceMemoryAllocator& allocator() const;
		DescriptorSetAllocator& descriptor_set_allocator() const;

		CmdBuffer<CmdBufferUsage::Disposable> create_disposable_cmd_buffer() const;

		const QueueFamily& queue_family(VkQueueFlags flags) const;
		const Queue& graphic_queue() const;
		Queue& graphic_queue();

		void wait_all_queues() const;

		ThreadDevicePtr thread_device() const;
		const DeviceResources& device_resources() const;
		DeviceResources& device_resources();

		const DeviceProperties& device_properties() const;

		LifetimeManager& lifetime_manager() const;

		VkDevice vk_device() const;
		const VkAllocationCallbacks* vk_allocation_callbacks() const;
		VkPhysicalDevice vk_physical_device() const;
		VkSampler vk_sampler(Sampler::Type type = Sampler::Repeat) const;

		const DebugUtils* debug_utils() const;
		const RayTracing* ray_tracing() const;


		template<typename T>
		decltype(auto) descriptor_set_layout(T&& t) const {
			return descriptor_set_allocator().descriptor_set_layout(y_fwd(t));
		}

		template<typename T>
		void destroy(T&& t) const {
			destroy_later(y_fwd(t));
		}

		template<typename T>
		void destroy_immediate(T&& t) const {
			_lifetime_manager.destroy_immediate(y_fwd(t));
		}

		template<typename T>
		void destroy_later(T&& t) const {
			_lifetime_manager.destroy_later(y_fwd(t));
		}

	private:
		Instance& _instance;
		Y_TODO(move this to the heap or slim it)
		PhysicalDevice _physical;

		core::Vector<QueueFamily> _queue_families;

		ScopedDevice _device;
		DeviceProperties _properties;

		mutable DeviceMemoryAllocator _allocator;
		mutable LifetimeManager _lifetime_manager;

		core::Vector<Queue> _queues;

		std::array<Sampler, 2> _samplers;

		mutable DescriptorSetAllocator _descriptor_set_allocator;

		mutable concurrent::SpinLock _lock;
		mutable core::Vector<std::unique_ptr<ThreadLocalDevice>> _thread_devices;

		struct {
			std::unique_ptr<RayTracing> raytracing;
		} _extensions;

		DeviceResources _resources;
};


template<typename T>
void DeviceLinked::destroy(T&& t) const {
	if(device()) {
		device()->destroy(y_fwd(t));
	}
}

template<typename T>
void DeviceLinked::destroy_immediate(T&& t) const {
	if(device()) {
		device()->destroy_immediate(y_fwd(t));
	}
}

template<typename T>
void ThreadDeviceLinked::destroy(T&& t) const {
	if(device()) {
		device()->destroy(y_fwd(t));
	}
}

template<typename T>
void ThreadDeviceLinked::destroy_immediate(T&& t) const {
	if(device()) {
		device()->destroy_immediate(y_fwd(t));
	}
}

}


#endif // YAVE_DEVICE_DEVICE_H
