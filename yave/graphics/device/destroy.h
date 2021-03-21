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
    /*if()*/ {
        vkDestroyBuffer(vk_device(), buffer, vk_allocation_callbacks());
    }
}

void vk_destroy(VkImage image) {
    /*if()*/ {
        vkDestroyImage(vk_device(), image, vk_allocation_callbacks());
    }
}

void vk_destroy(VkImageView image_view) {
    /*if()*/ {
        vkDestroyImageView(vk_device(), image_view, vk_allocation_callbacks());
    }
}

void vk_destroy(VkRenderPass render_pass) {
    /*if()*/ {
        vkDestroyRenderPass(vk_device(), render_pass, vk_allocation_callbacks());
    }
}

void vk_destroy(VkFramebuffer framebuffer) {
    /*if()*/ {
        vkDestroyFramebuffer(vk_device(), framebuffer, vk_allocation_callbacks());
    }
}

void vk_destroy(VkPipeline pipeline) {
    /*if()*/ {
        vkDestroyPipeline(vk_device(), pipeline, vk_allocation_callbacks());
    }
}

void vk_destroy(VkPipelineLayout pipeline_layout) {
    /*if()*/ {
        vkDestroyPipelineLayout(vk_device(), pipeline_layout, vk_allocation_callbacks());
    }
}

void vk_destroy(VkShaderModule module) {
    /*if()*/ {
        vkDestroyShaderModule(vk_device(), module, vk_allocation_callbacks());
    }
}

void vk_destroy(VkSampler sampler) {
    /*if()*/ {
        vkDestroySampler(vk_device(), sampler, vk_allocation_callbacks());
    }
}

void vk_destroy(VkSwapchainKHR swapchain) {
    /*if()*/ {
        vkDestroySwapchainKHR(vk_device(), swapchain, vk_allocation_callbacks());
    }
}

void vk_destroy(VkCommandPool pool) {
    /*if()*/ {
        vkDestroyCommandPool(vk_device(), pool, vk_allocation_callbacks());
    }
}

void vk_destroy(VkFence fence) {
    /*if()*/ {
        vkDestroyFence(vk_device(), fence, vk_allocation_callbacks());
    }
}

void vk_destroy(VkDescriptorPool pool) {
    /*if()*/ {
        vkDestroyDescriptorPool(vk_device(), pool, vk_allocation_callbacks());
    }
}

void vk_destroy(VkDescriptorSetLayout layout) {
    /*if()*/ {
        vkDestroyDescriptorSetLayout(vk_device(), layout, vk_allocation_callbacks());
    }
}

void vk_destroy(VkSemaphore semaphore) {
    /*if()*/ {
        vkDestroySemaphore(vk_device(), semaphore, vk_allocation_callbacks());
    }
}

void vk_destroy(VkQueryPool pool) {
    /*if()*/ {
        vkDestroyQueryPool(vk_device(), pool, vk_allocation_callbacks());
    }
}

void vk_destroy(VkEvent event) {
    /*if()*/ {
        vkDestroyEvent(vk_device(), event, vk_allocation_callbacks());
    }
}




void vk_destroy(VkSurfaceKHR surface) {
    /*if()*/ {
        vkDestroySurfaceKHR(vk_device_instance(), surface, vk_allocation_callbacks());
    }
}

}

#endif // YAVE_GRAPHICS_DEVICE_DESTROY_H

