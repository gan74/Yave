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

#include "destroy.h"
#include <yave/device/Device.h>

namespace yave {
namespace detail {

void destroy(DevicePtr dptr, VkBuffer buffer) {
	/*if(dptr)*/ {
		vkDestroyBuffer(dptr->vk_device(), buffer, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkImage image) {
	/*if(dptr)*/ {
		vkDestroyImage(dptr->vk_device(), image, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkImageView image_view) {
	/*if(dptr)*/ {
		vkDestroyImageView(dptr->vk_device(), image_view, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkRenderPass render_pass) {
	/*if(dptr)*/ {
		vkDestroyRenderPass(dptr->vk_device(), render_pass, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkFramebuffer framebuffer) {
	/*if(dptr)*/ {
		vkDestroyFramebuffer(dptr->vk_device(), framebuffer, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkPipeline pipeline) {
	/*if(dptr)*/ {
		vkDestroyPipeline(dptr->vk_device(), pipeline, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkPipelineLayout pipeline_layout) {
	/*if(dptr)*/ {
		vkDestroyPipelineLayout(dptr->vk_device(), pipeline_layout, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkShaderModule module) {
	/*if(dptr)*/ {
		vkDestroyShaderModule(dptr->vk_device(), module, dptr->vk_allocation_callbacks());
	}
}


void destroy(DevicePtr dptr, VkSampler sampler) {
	/*if(dptr)*/ {
		vkDestroySampler(dptr->vk_device(), sampler, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkSwapchainKHR swapchain) {
	/*if(dptr)*/ {
		vkDestroySwapchainKHR(dptr->vk_device(), swapchain, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkCommandPool pool) {
	/*if(dptr)*/ {
		vkDestroyCommandPool(dptr->vk_device(), pool, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkFence fence) {
	/*if(dptr)*/ {
		vkDestroyFence(dptr->vk_device(), fence, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkDescriptorPool pool) {
	/*if(dptr)*/ {
		vkDestroyDescriptorPool(dptr->vk_device(), pool, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkDescriptorSetLayout layout) {
	/*if(dptr)*/ {
		vkDestroyDescriptorSetLayout(dptr->vk_device(), layout, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkSemaphore semaphore) {
	/*if(dptr)*/ {
		vkDestroySemaphore(dptr->vk_device(), semaphore, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkQueryPool pool) {
	/*if(dptr)*/ {
		vkDestroyQueryPool(dptr->vk_device(), pool, dptr->vk_allocation_callbacks());
	}
}

void destroy(DevicePtr dptr, VkEvent event) {
	/*if(dptr)*/ {
		vkDestroyEvent(dptr->vk_device(), event, dptr->vk_allocation_callbacks());
	}
}




void destroy(DevicePtr dptr, VkSurfaceKHR surface) {
	/*if(dptr)*/ {
		vkDestroySurfaceKHR(dptr->instance().vk_instance(), surface, dptr->vk_allocation_callbacks());
	}
}

}
}
