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

#include "Instance.h"
#include "Queue.h"
#include "DeviceProperties.h"
#include "PhysicalDevice.h"
#include "ThreadLocalDevice.h"
#include "DeviceResources.h"
#include "LifetimeManager.h"

#include <yave/graphics/descriptors/DescriptorSetAllocator.h>

#include <yave/graphics/images/Sampler.h>
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
        Device(Instance& instance, PhysicalDevice device);
        ~Device();

        const PhysicalDevice& physical_device() const;
        const Instance& instance() const;

        DeviceMemoryAllocator& allocator() const;
        DescriptorSetAllocator& descriptor_set_allocator() const;

        CmdBuffer create_disposable_cmd_buffer() const;

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
        VkSampler vk_sampler(SamplerType type = SamplerType::LinearRepeat) const;

        static VkPhysicalDeviceFeatures required_device_features();

        const DebugUtils* debug_utils() const;
        const RayTracing* ray_tracing() const;

    private:
        Instance& _instance;

        Y_TODO(move this to the heap or slim it)
        PhysicalDevice _physical;

        u32 _graphic_queue_index = 0;

        ScopedDevice _device;
        DeviceProperties _properties;

        mutable DeviceMemoryAllocator _allocator;
        mutable LifetimeManager _lifetime_manager;

        Queue _graphic_queue;

        std::array<Sampler, 4> _samplers;

        mutable DescriptorSetAllocator _descriptor_set_allocator;

        mutable concurrent::SpinLock _lock;
        mutable core::Vector<std::unique_ptr<ThreadLocalDevice>> _thread_devices;

        struct {
            std::unique_ptr<RayTracing> raytracing;
        } _extensions;

        DeviceResources _resources;
};

}


#endif // YAVE_DEVICE_DEVICE_H

