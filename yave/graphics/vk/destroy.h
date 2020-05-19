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
along with this program. If not, see <http://www.gnu.org/licenses/>.
**********************************/
#ifndef YAVE_GRAPHICS_VK_DESTROY_H
#define YAVE_GRAPHICS_VK_DESTROY_H

#include "vk.h"

namespace yave {
namespace detail {

void destroy(DevicePtr dptr, VkBuffer buffer);
void destroy(DevicePtr dptr, VkImage image);
void destroy(DevicePtr dptr, VkImageView image_view);
void destroy(DevicePtr dptr, VkRenderPass render_pass);
void destroy(DevicePtr dptr, VkFramebuffer framebuffer);
void destroy(DevicePtr dptr, VkPipeline pipeline);
void destroy(DevicePtr dptr, VkPipelineLayout pipeline_layout);
void destroy(DevicePtr dptr, VkShaderModule module);
void destroy(DevicePtr dptr, VkSampler sampler);
void destroy(DevicePtr dptr, VkSwapchainKHR swapchain);
void destroy(DevicePtr dptr, VkCommandPool pool);
void destroy(DevicePtr dptr, VkFence fence);
void destroy(DevicePtr dptr, VkDescriptorPool pool);
void destroy(DevicePtr dptr, VkDescriptorSetLayout layout);
void destroy(DevicePtr dptr, VkSemaphore semaphore);
void destroy(DevicePtr dptr, VkQueryPool pool);
void destroy(DevicePtr dptr, VkEvent event);

void destroy(DevicePtr dptr, VkSurfaceKHR surface);

}
}

#endif // YAVE_GRAPHICS_VK_DESTROY_H
