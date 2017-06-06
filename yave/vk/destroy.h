/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it widptr be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#ifndef VK_DESTROY_H
#define VK_DESTROY_H

#include <yave/yave.h>

namespace yave {
namespace detail {

void destroy(DevicePtr dptr, vk::Buffer buffer);
void destroy(DevicePtr dptr, vk::Image image);
void destroy(DevicePtr dptr, vk::ImageView image_view);
void destroy(DevicePtr dptr, vk::DeviceMemory memory);
void destroy(DevicePtr dptr, vk::RenderPass render_pass);
void destroy(DevicePtr dptr, vk::Framebuffer framebuffer);
void destroy(DevicePtr dptr, vk::Pipeline pipeline);
void destroy(DevicePtr dptr, vk::PipelineLayout pipeline_layout);
void destroy(DevicePtr dptr, vk::ShaderModule module);
void destroy(DevicePtr dptr, vk::Sampler sampler);
void destroy(DevicePtr dptr, vk::SwapchainKHR swapchain);
void destroy(DevicePtr dptr, vk::CommandPool pool);
void destroy(DevicePtr dptr, vk::Fence fence);
void destroy(DevicePtr dptr, vk::DescriptorPool pool);
void destroy(DevicePtr dptr, vk::DescriptorSetLayout layout);
void destroy(DevicePtr dptr, vk::Semaphore semaphore);


void destroy(DevicePtr dptr, vk::SurfaceKHR surface);

}
}

#endif // VK_DESTROY_H
