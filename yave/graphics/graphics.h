/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include <y/core/Span.h>

namespace yave {

void init_device(Instance& instance);
void init_device(Instance& instance, PhysicalDevice device);

void destroy_device();

bool device_initialized();

const InstanceParams& instance_params();

VkDevice vk_device();
VkInstance vk_device_instance();
VkPhysicalDevice vk_physical_device();

CmdBufferRecorder create_disposable_cmd_buffer();
ComputeCmdBufferRecorder create_disposable_compute_cmd_buffer();
TransferCmdBufferRecorder create_disposable_transfer_cmd_buffer();

const PhysicalDevice& physical_device();
DeviceMemoryAllocator& device_allocator();
DescriptorLayoutAllocator& layout_allocator();
MeshAllocator& mesh_allocator();
MaterialAllocator& material_allocator();
TextureLibrary& texture_library();
CmdQueue& command_queue();
CmdQueue& loading_command_queue();
const DeviceResources& device_resources();
const DeviceProperties& device_properties();
bool raytracing_enabled();
LifetimeManager& lifetime_manager();

const VkAllocationCallbacks* vk_allocation_callbacks();
VkSampler vk_sampler(SamplerType type);

const DebugUtils* debug_utils();
const DiagnosticCheckpoints* diagnostic_checkpoints();

void wait_all_queues();

#define YAVE_GENERATE_DESTROY(T) void destroy_graphic_resource(T&& t);
YAVE_GRAPHIC_HANDLE_TYPES(YAVE_GENERATE_DESTROY)
#undef YAVE_GENERATE_DESTROY

}


#endif // YAVE_GRAPHICS_GRAPHICS_H

