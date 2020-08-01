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

#include "utils.h"

#include <yave/graphics/device/Device.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

VkDevice vk_device(DevicePtr dptr) {
    return dptr->vk_device();
}

VkInstance vk_device_instance(DevicePtr dptr) {
    return dptr->instance().vk_instance();
}


const PhysicalDevice& physical_device(DevicePtr dptr) {
    return dptr->physical_device();
}

CmdBuffer create_disposable_cmd_buffer(DevicePtr dptr) {
    return dptr->create_disposable_cmd_buffer();
}

DeviceMemoryAllocator& device_allocator(DevicePtr dptr) {
    return dptr->allocator();
}

DescriptorSetAllocator& descriptor_set_allocator(DevicePtr dptr) {
    return dptr->descriptor_set_allocator();
}

const Queue& graphic_queue(DevicePtr dptr) {
    return dptr->graphic_queue();
}

const DeviceResources& device_resources(DevicePtr dptr) {
    return dptr->device_resources();
}

const DeviceProperties& device_properties(DevicePtr dptr) {
    return dptr->device_properties();
}

LifetimeManager& lifetime_manager(DevicePtr dptr) {
    return dptr->lifetime_manager();
}

const VkAllocationCallbacks* vk_allocation_callbacks(DevicePtr dptr) {
    return dptr->vk_allocation_callbacks();
}

VkSampler vk_sampler(DevicePtr dptr, SamplerType type) {
    return dptr->vk_sampler(type);
}

const QueueFamily& queue_family(DevicePtr dptr, VkQueueFlags flags) {
    return dptr->queue_family(flags);
}

const DebugUtils* debug_utils(DevicePtr dptr) {
    return dptr ? dptr->debug_utils() : nullptr;
}

const RayTracing* ray_tracing(DevicePtr dptr) {
    return dptr ? dptr->ray_tracing() : nullptr;
}

void wait_all_queues(DevicePtr dptr) {
    dptr->wait_all_queues();
}


#define YAVE_GENERATE_DESTROY_IMPL(T)                                   \
    void device_destroy(DevicePtr dptr, T t) {                          \
        if(dptr) {                                                      \
            lifetime_manager(dptr).destroy_later(std::move(t));         \
        }                                                               \
    }

YAVE_GRAPHIC_RESOURCE_TYPES(YAVE_GENERATE_DESTROY_IMPL)
#undef YAVE_GENERATE_DESTROY_IMPL

}

