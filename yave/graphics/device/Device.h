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
#ifndef YAVE_DEVICE_DEVICE_H
#define YAVE_DEVICE_DEVICE_H

#include <yave/yave.h>

#include "Instance.h"
#include "Queue.h"
#include "DeviceProperties.h"
#include "PhysicalDevice.h"
#include "ThreadLocalDevice.h"
#include "DeviceResources.h"
#include "LifetimeManager.h"

#include <yave/graphics/images/Sampler.h>
#include <yave/graphics/descriptors/DescriptorSetAllocator.h>
#include <yave/graphics/memory/DeviceMemoryAllocator.h>

#include <y/concurrent/VectorLock.h>

#include <thread>

namespace yave {

Y_TODO(const corrrectness?)
class Device : NonMovable {

    public:
        explicit Device(Instance& instance);

        Device(Instance& instance, PhysicalDevice device);
        ~Device();

        static DevicePtr main_device();

        void wait_all_queues() const;
        concurrent::VectorLock<std::mutex> lock_all_queues_and_wait() const;

        const PhysicalDevice& physical_device() const;
        const Instance& instance() const;
        ThreadDevicePtr thread_device() const;

        VkDevice vk_device() const;
        VkPhysicalDevice vk_physical_device() const;
        VkSampler vk_sampler(SamplerType type = SamplerType::LinearRepeat) const;
        const VkAllocationCallbacks* vk_allocation_callbacks() const;

        LifetimeManager& lifetime_manager() const;

        DeviceMemoryAllocator& allocator() const;
        DescriptorSetAllocator& descriptor_set_allocator() const;

        const Queue& graphic_queue() const;
        const Queue& loading_queue() const;

        const DeviceResources& device_resources() const;
        DeviceResources& device_resources();

        const DeviceProperties& device_properties() const;

        static VkPhysicalDeviceFeatures required_device_features();
        static VkPhysicalDeviceVulkan11Features required_device_features_1_1();
        static VkPhysicalDeviceVulkan12Features required_device_features_1_2();

        static bool has_required_features(const PhysicalDevice& physical);
        static bool has_required_properties(const PhysicalDevice& physical);


        const DebugUtils* debug_utils() const;
        const RayTracing* ray_tracing() const;

    private:
        void create_thread_device(ThreadDevicePtr* dptr) const;
        void destroy_thread_device(ThreadDevicePtr device) const;

        static Device* _main_device;


        VkDevice _device = {};

        Queue _graphic_queue;
        Queue _loading_queue;

        mutable Uninitialized<DeviceMemoryAllocator> _allocator;
        mutable Uninitialized<LifetimeManager> _lifetime_manager;
        mutable Uninitialized<DescriptorSetAllocator> _descriptor_set_allocator;

        std::array<Uninitialized<Sampler>, 5> _samplers;

        mutable std::mutex _devices_lock;
        mutable core::Vector<std::pair<std::unique_ptr<ThreadLocalDevice>, ThreadDevicePtr*>> _thread_devices;

        Uninitialized<DeviceResources> _resources;

        u32 _main_queue_index = 0;

        Instance& _instance;
        std::unique_ptr<PhysicalDevice> _physical;

        DeviceProperties _properties = {};
};

}


#endif // YAVE_DEVICE_DEVICE_H

