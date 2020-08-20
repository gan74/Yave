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

#include "ImageUsage.h"

namespace yave {

static VkImageLayout vk_layout(ImageUsage usage) {
    if((usage & ImageUsage::SwapchainBit) != ImageUsage::None) {
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    if((usage & ImageUsage::StorageBit) != ImageUsage::None) {
        return VK_IMAGE_LAYOUT_GENERAL;
    }

    Y_TODO(maybe transition images before and after copies?)
    const bool texture = (usage & ImageUsage::TextureBit) != ImageUsage::None;
    if((usage & ImageUsage::TransferSrcBit) != ImageUsage::None) {
        return texture ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    }
    if((usage & ImageUsage::TransferDstBit) != ImageUsage::None) {
        return texture ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    if(texture) {
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    y_fatal("Undefined image layout.");
}

VkImageLayout vk_image_layout(ImageUsage usage) {
    switch(usage) {
        case ImageUsage::ColorBit:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        case ImageUsage::DepthBit:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        case ImageUsage::DepthTexture:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        case ImageUsage::TextureBit:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        case ImageUsage::SwapchainBit:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        default:
            break;
    }

    return vk_layout(usage);
}


}

