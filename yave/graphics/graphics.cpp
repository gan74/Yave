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

#include "graphics.h"

#include <yave/graphics/device/Device.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

DevicePtr main_device() {
    return Device::main_device();
}

ThreadDevicePtr thread_device() {
    return main_device()->thread_device();
}

VkDevice vk_device() {
    return main_device()->vk_device();
}

VkInstance vk_device_instance() {
    return main_device()->instance().vk_instance();
}

VkPhysicalDevice vk_physical_device() {
    return main_device()->physical_device().vk_physical_device();
}

const PhysicalDevice& physical_device() {
    return main_device()->physical_device();
}

CmdBuffer create_disposable_cmd_buffer() {
    return thread_device()->create_disposable_cmd_buffer();
}

DeviceMemoryAllocator& device_allocator() {
    return main_device()->allocator();
}

DescriptorSetAllocator& descriptor_set_allocator() {
    return main_device()->descriptor_set_allocator();
}

const Queue& graphic_queue() {
    return main_device()->graphic_queue();
}

const Queue& loading_queue() {
    return main_device()->loading_queue();
}

const DeviceResources& device_resources() {
    return main_device()->device_resources();
}

const DeviceProperties& device_properties() {
    return main_device()->device_properties();
}

LifetimeManager& lifetime_manager() {
    return main_device()->lifetime_manager();
}

const VkAllocationCallbacks* vk_allocation_callbacks() {
    return main_device()->vk_allocation_callbacks();
}

VkSampler vk_sampler(SamplerType type) {
    return main_device()->vk_sampler(type);
}

const DebugUtils* debug_utils() {
    return main_device()->debug_utils();
}

const RayTracing* ray_tracing() {
    return main_device()->ray_tracing();
}

void wait_all_queues() {
    main_device()->wait_all_queues();
}


#define YAVE_GENERATE_DESTROY_IMPL(T)                                                   \
    void device_destroy(T t) {                                                          \
        if(t) {                                                                         \
            lifetime_manager().destroy_later(std::move(t));                             \
        }                                                                               \
    }

YAVE_VK_RESOURCE_TYPES(YAVE_GENERATE_DESTROY_IMPL)
#undef YAVE_GENERATE_DESTROY_IMPL

#define YAVE_GENERATE_DESTROY_IMPL(T)                                                   \
    void device_destroy(T t) {                                                          \
        if(!t.is_null()) {                                                              \
            lifetime_manager().destroy_later(std::move(t));                             \
        }                                                                               \
    }

YAVE_YAVE_RESOURCE_TYPES(YAVE_GENERATE_DESTROY_IMPL)
#undef YAVE_GENERATE_DESTROY_IMPL

}

