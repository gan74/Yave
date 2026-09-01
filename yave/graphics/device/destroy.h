/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

