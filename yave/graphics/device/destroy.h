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
#ifndef YAVE_GRAPHICS_DEVICE_DESTROY_H
#define YAVE_GRAPHICS_DEVICE_DESTROY_H

#include <yave/graphics/graphics.h>

namespace yave {

void vk_destroy(VkBuffer buffer) {
    vkDestroyBuffer(vk_device(), buffer, vk_allocation_callbacks());
}

void vk_destroy(VkImage image) {
    vkDestroyImage(vk_device(), image, vk_allocation_callbacks());
}

void vk_destroy(VkImageView image_view) {
    vkDestroyImageView(vk_device(), image_view, vk_allocation_callbacks());
}

void vk_destroy(VkRenderPass render_pass) {
    vkDestroyRenderPass(vk_device(), render_pass, vk_allocation_callbacks());
}

void vk_destroy(VkFramebuffer framebuffer) {
    vkDestroyFramebuffer(vk_device(), framebuffer, vk_allocation_callbacks());
}

void vk_destroy(VkPipeline pipeline) {
    vkDestroyPipeline(vk_device(), pipeline, vk_allocation_callbacks());
}

void vk_destroy(VkPipelineLayout pipeline_layout) {
    vkDestroyPipelineLayout(vk_device(), pipeline_layout, vk_allocation_callbacks());
}

void vk_destroy(VkShaderModule module) {
    vkDestroyShaderModule(vk_device(), module, vk_allocation_callbacks());
}

void vk_destroy(VkSampler sampler) {
    vkDestroySampler(vk_device(), sampler, vk_allocation_callbacks());
}

void vk_destroy(VkSwapchainKHR swapchain) {
    vkDestroySwapchainKHR(vk_device(), swapchain, vk_allocation_callbacks());
}

void vk_destroy(VkCommandPool pool) {
    vkDestroyCommandPool(vk_device(), pool, vk_allocation_callbacks());
}

void vk_destroy(VkFence fence) {
    vkDestroyFence(vk_device(), fence, vk_allocation_callbacks());
}

void vk_destroy(VkDescriptorPool pool) {
    vkDestroyDescriptorPool(vk_device(), pool, vk_allocation_callbacks());
}

void vk_destroy(VkDescriptorSetLayout layout) {
    vkDestroyDescriptorSetLayout(vk_device(), layout, vk_allocation_callbacks());
}

void vk_destroy(VkSemaphore semaphore) {
    vkDestroySemaphore(vk_device(), semaphore, vk_allocation_callbacks());
}

void vk_destroy(VkQueryPool pool) {
    vkDestroyQueryPool(vk_device(), pool, vk_allocation_callbacks());
}

void vk_destroy(VkEvent event) {
    vkDestroyEvent(vk_device(), event, vk_allocation_callbacks());
}




void vk_destroy(VkSurfaceKHR surface) {
    vkDestroySurfaceKHR(vk_device_instance(), surface, vk_allocation_callbacks());
}




void vk_destroy(VkAccelerationStructureKHR acc) {
    vkDestroyAccelerationStructureKHR(vk_device(), acc, vk_allocation_callbacks());
}

}

#endif // YAVE_GRAPHICS_DEVICE_DESTROY_H

