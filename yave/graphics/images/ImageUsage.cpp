/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include <y/utils/format.h>

namespace yave {

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

        case ImageUsage::TransferSrcBit:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        case ImageUsage::TransferDstBit:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        default:
            break;
    }

    if((usage & ImageUsage::SwapchainBit) != ImageUsage::None) {
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    if((usage & ImageUsage::StorageBit) != ImageUsage::None) {
        return VK_IMAGE_LAYOUT_GENERAL;
    }

    if((usage & ImageUsage::TextureBit) != ImageUsage::None) {
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    if((usage & ImageUsage::DepthBit) != ImageUsage::None) {
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    if((usage & ImageUsage::ColorBit) != ImageUsage::None) {
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    
    y_fatal("Unsupported image usage ({})", usage);
}

}

