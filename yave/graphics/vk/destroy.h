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

void vk_destroy(DevicePtr dptr, VkBuffer buffer);
void vk_destroy(DevicePtr dptr, VkImage image);
void vk_destroy(DevicePtr dptr, VkImageView image_view);
void vk_destroy(DevicePtr dptr, VkRenderPass render_pass);
void vk_destroy(DevicePtr dptr, VkFramebuffer framebuffer);
void vk_destroy(DevicePtr dptr, VkPipeline pipeline);
void vk_destroy(DevicePtr dptr, VkPipelineLayout pipeline_layout);
void vk_destroy(DevicePtr dptr, VkShaderModule module);
void vk_destroy(DevicePtr dptr, VkSampler sampler);
void vk_destroy(DevicePtr dptr, VkSwapchainKHR swapchain);
void vk_destroy(DevicePtr dptr, VkCommandPool pool);
void vk_destroy(DevicePtr dptr, VkFence fence);
void vk_destroy(DevicePtr dptr, VkDescriptorPool pool);
void vk_destroy(DevicePtr dptr, VkDescriptorSetLayout layout);
void vk_destroy(DevicePtr dptr, VkSemaphore semaphore);
void vk_destroy(DevicePtr dptr, VkQueryPool pool);
void vk_destroy(DevicePtr dptr, VkEvent event);

void vk_destroy(DevicePtr dptr, VkSurfaceKHR surface);

}
}

#endif // YAVE_GRAPHICS_VK_DESTROY_H

