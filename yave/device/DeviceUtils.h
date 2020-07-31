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
#ifndef YAVE_DEVICE_DEVICE_UTILS_H
#define YAVE_DEVICE_DEVICE_UTILS_H


#include <yave/yave.h>

#include <yave/graphics/vk/vk.h>
#include <yave/graphics/images/SamplerType.h>

namespace yave {

VkDevice vk_device(DevicePtr dptr);
VkInstance vk_device_instance(DevicePtr dptr);

CmdBuffer create_disposable_cmd_buffer(DevicePtr dptr);

const PhysicalDevice& physical_device(DevicePtr dptr);
DeviceMemoryAllocator& device_allocator(DevicePtr dptr);
DescriptorSetAllocator& descriptor_set_allocator(DevicePtr dptr);
const Queue& graphic_queue(DevicePtr dptr);
const DeviceResources& device_resources(DevicePtr dptr);
const DeviceProperties& device_properties(DevicePtr dptr);
LifetimeManager& lifetime_manager(DevicePtr dptr);

const VkAllocationCallbacks* vk_allocation_callbacks(DevicePtr dptr);
VkSampler vk_sampler(DevicePtr dptr, SamplerType type);
const QueueFamily& queue_family(DevicePtr dptr, VkQueueFlags flags);

const DebugUtils* debug_utils(DevicePtr dptr);
const RayTracing* ray_tracing(DevicePtr dptr);

void wait_all_queues(DevicePtr dptr);

}


#endif // YAVE_DEVICE_DEVICE_UTILS_H

