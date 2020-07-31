/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef YAVE_DEVICE_DEVICEHANDLE_H
#define YAVE_DEVICE_DEVICEHANDLE_H

#include <yave/yave.h>
#include <yave/graphics/vk/vk.h>

namespace yave {

#define YAVE_DEVICE_RESOURCE_TYPES(X)       \
    X(DeviceMemory)                         \
    X(DescriptorSetData)                    \
    X(VkBuffer)                             \
    X(VkImage)                              \
    X(VkImageView)                          \
    X(VkRenderPass)                         \
    X(VkFramebuffer)                        \
    X(VkPipeline)                           \
    X(VkPipelineLayout)                     \
    X(VkShaderModule)                       \
    X(VkSampler)                            \
    X(VkSwapchainKHR)                       \
    X(VkCommandPool)                        \
    X(VkFence)                              \
    X(VkDescriptorPool)                     \
    X(VkDescriptorSetLayout)                \
    X(VkSemaphore)                          \
    X(VkQueryPool)                          \
    X(VkEvent)                              \
    X(VkSurfaceKHR)                         


#define YAVE_GENERATE_DESTROY(T) void device_destroy(DevicePtr dptr, T t);              
    
YAVE_DEVICE_RESOURCE_TYPES(YAVE_GENERATE_DESTROY)

}

#endif // YAVE_DEVICE_DEVICEHANDLE_H

