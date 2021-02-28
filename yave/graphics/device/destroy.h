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

#include <yave/graphics/utils.h>

namespace yave {

void vk_destroy(DevicePtr dptr, VkBuffer buffer) {
    /*if(dptr)*/ {
        vkDestroyBuffer(vk_device(dptr), buffer, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkImage image) {
    /*if(dptr)*/ {
        vkDestroyImage(vk_device(dptr), image, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkImageView image_view) {
    /*if(dptr)*/ {
        vkDestroyImageView(vk_device(dptr), image_view, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkRenderPass render_pass) {
    /*if(dptr)*/ {
        vkDestroyRenderPass(vk_device(dptr), render_pass, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkFramebuffer framebuffer) {
    /*if(dptr)*/ {
        vkDestroyFramebuffer(vk_device(dptr), framebuffer, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkPipeline pipeline) {
    /*if(dptr)*/ {
        vkDestroyPipeline(vk_device(dptr), pipeline, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkPipelineLayout pipeline_layout) {
    /*if(dptr)*/ {
        vkDestroyPipelineLayout(vk_device(dptr), pipeline_layout, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkShaderModule module) {
    /*if(dptr)*/ {
        vkDestroyShaderModule(vk_device(dptr), module, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkSampler sampler) {
    /*if(dptr)*/ {
        vkDestroySampler(vk_device(dptr), sampler, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkSwapchainKHR swapchain) {
    /*if(dptr)*/ {
        vkDestroySwapchainKHR(vk_device(dptr), swapchain, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkCommandPool pool) {
    /*if(dptr)*/ {
        vkDestroyCommandPool(vk_device(dptr), pool, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkFence fence) {
    /*if(dptr)*/ {
        vkDestroyFence(vk_device(dptr), fence, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkDescriptorPool pool) {
    /*if(dptr)*/ {
        vkDestroyDescriptorPool(vk_device(dptr), pool, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkDescriptorSetLayout layout) {
    /*if(dptr)*/ {
        vkDestroyDescriptorSetLayout(vk_device(dptr), layout, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkSemaphore semaphore) {
    /*if(dptr)*/ {
        vkDestroySemaphore(vk_device(dptr), semaphore, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkQueryPool pool) {
    /*if(dptr)*/ {
        vkDestroyQueryPool(vk_device(dptr), pool, vk_allocation_callbacks(dptr));
    }
}

void vk_destroy(DevicePtr dptr, VkEvent event) {
    /*if(dptr)*/ {
        vkDestroyEvent(vk_device(dptr), event, vk_allocation_callbacks(dptr));
    }
}




void vk_destroy(DevicePtr dptr, VkSurfaceKHR surface) {
    /*if(dptr)*/ {
        vkDestroySurfaceKHR(vk_device_instance(dptr), surface, vk_allocation_callbacks(dptr));
    }
}

}

#endif // YAVE_GRAPHICS_DEVICE_DESTROY_H

