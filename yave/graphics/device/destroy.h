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

void vk_destroy(DevicePtr dptr, VkBuffer buffer) {
    /*if(dptr)*/ {
        vkDestroyBuffer(vk_device(), buffer, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkImage image) {
    /*if(dptr)*/ {
        vkDestroyImage(vk_device(), image, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkImageView image_view) {
    /*if(dptr)*/ {
        vkDestroyImageView(vk_device(), image_view, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkRenderPass render_pass) {
    /*if(dptr)*/ {
        vkDestroyRenderPass(vk_device(), render_pass, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkFramebuffer framebuffer) {
    /*if(dptr)*/ {
        vkDestroyFramebuffer(vk_device(), framebuffer, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkPipeline pipeline) {
    /*if(dptr)*/ {
        vkDestroyPipeline(vk_device(), pipeline, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkPipelineLayout pipeline_layout) {
    /*if(dptr)*/ {
        vkDestroyPipelineLayout(vk_device(), pipeline_layout, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkShaderModule module) {
    /*if(dptr)*/ {
        vkDestroyShaderModule(vk_device(), module, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkSampler sampler) {
    /*if(dptr)*/ {
        vkDestroySampler(vk_device(), sampler, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkSwapchainKHR swapchain) {
    /*if(dptr)*/ {
        vkDestroySwapchainKHR(vk_device(), swapchain, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkCommandPool pool) {
    /*if(dptr)*/ {
        vkDestroyCommandPool(vk_device(), pool, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkFence fence) {
    /*if(dptr)*/ {
        vkDestroyFence(vk_device(), fence, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkDescriptorPool pool) {
    /*if(dptr)*/ {
        vkDestroyDescriptorPool(vk_device(), pool, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkDescriptorSetLayout layout) {
    /*if(dptr)*/ {
        vkDestroyDescriptorSetLayout(vk_device(), layout, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkSemaphore semaphore) {
    /*if(dptr)*/ {
        vkDestroySemaphore(vk_device(), semaphore, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkQueryPool pool) {
    /*if(dptr)*/ {
        vkDestroyQueryPool(vk_device(), pool, vk_allocation_callbacks());
    }
}

void vk_destroy(DevicePtr dptr, VkEvent event) {
    /*if(dptr)*/ {
        vkDestroyEvent(vk_device(), event, vk_allocation_callbacks());
    }
}




void vk_destroy(DevicePtr dptr, VkSurfaceKHR surface) {
    /*if(dptr)*/ {
        vkDestroySurfaceKHR(vk_device_instance(), surface, vk_allocation_callbacks());
    }
}

}

#endif // YAVE_GRAPHICS_DEVICE_DESTROY_H

