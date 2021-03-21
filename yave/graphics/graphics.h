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
#ifndef YAVE_GRAPHICS_GRAPHICS_H
#define YAVE_GRAPHICS_GRAPHICS_H

#include <yave/yave.h>

#include <yave/graphics/vk/vk.h>
#include <yave/graphics/device/ResourceType.h>
#include <yave/graphics/images/SamplerType.h>

namespace yave {

DevicePtr main_device();

VkDevice vk_device();
VkInstance vk_device_instance();

CmdBuffer create_disposable_cmd_buffer();

const PhysicalDevice& physical_device();
DeviceMemoryAllocator& device_allocator();
DescriptorSetAllocator& descriptor_set_allocator();
const Queue& graphic_queue();
const Queue& loading_queue();
const DeviceResources& device_resources();
const DeviceProperties& device_properties();
LifetimeManager& lifetime_manager();

const VkAllocationCallbacks* vk_allocation_callbacks();
VkSampler vk_sampler(SamplerType type);

const DebugUtils* debug_utils();
const RayTracing* ray_tracing();

void wait_all_queues();

#define YAVE_GENERATE_DESTROY(T) void device_destroy(T t);
YAVE_GRAPHIC_RESOURCE_TYPES(YAVE_GENERATE_DESTROY)
#undef YAVE_GENERATE_DESTROY

}


#endif // YAVE_GRAPHICS_GRAPHICS_H

