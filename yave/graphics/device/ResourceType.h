/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_DEVICE_RESOURCETYPE_H
#define YAVE_GRAPHICS_DEVICE_RESOURCETYPE_H

#include <yave/yave.h>
#include <yave/graphics/graphics.h>

namespace yave {

#define YAVE_VK_HANDLES_TYPES(X)                \
    X(VkHandle<VkBuffer>)                       \
    X(VkHandle<VkImage>)                        \
    X(VkHandle<VkImageView>)                    \
    X(VkHandle<VkRenderPass>)                   \
    X(VkHandle<VkFramebuffer>)                  \
    X(VkHandle<VkPipeline>)                     \
    X(VkHandle<VkPipelineLayout>)               \
    X(VkHandle<VkShaderModule>)                 \
    X(VkHandle<VkSampler>)                      \
    X(VkHandle<VkSwapchainKHR>)                 \
    X(VkHandle<VkCommandPool>)                  \
    X(VkHandle<VkFence>)                        \
    X(VkHandle<VkDescriptorPool>)               \
    X(VkHandle<VkDescriptorSetLayout>)          \
    X(VkHandle<VkSemaphore>)                    \
    X(VkHandle<VkQueryPool>)                    \
    X(VkHandle<VkEvent>)                        \
    X(VkHandle<VkSurfaceKHR>)                   \
    X(VkHandle<VkAccelerationStructureKHR>)

#define YAVE_YAVE_RESOURCE_TYPES(X)             \
    X(DeviceMemory)                             \
    X(DescriptorSetData)                        \
    X(MeshDrawData)                             \
    X(MaterialDrawData)

#define YAVE_GRAPHIC_RESOURCE_TYPES(X)          \
    YAVE_YAVE_RESOURCE_TYPES(X)                 \
    YAVE_VK_RESOURCE_TYPES(X)

#define YAVE_GRAPHIC_HANDLE_TYPES(X)            \
    YAVE_YAVE_RESOURCE_TYPES(X)                 \
    YAVE_VK_HANDLES_TYPES(X)

}

#endif // YAVE_GRAPHICS_DEVICE_RESOURCETYPE_H

