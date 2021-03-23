/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_IMAGES_IMAGEUSAGE_H
#define YAVE_GRAPHICS_IMAGES_IMAGEUSAGE_H

#include "ImageFormat.h"

#include <algorithm>

namespace yave {

enum class ImageUsage : u32 {
    None = 0,

    TextureBit      = VK_IMAGE_USAGE_SAMPLED_BIT,
    DepthBit        = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    ColorBit        = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    StorageBit      = VK_IMAGE_USAGE_STORAGE_BIT,

    TransferSrcBit  = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    TransferDstBit  = VK_IMAGE_USAGE_TRANSFER_DST_BIT,

    SwapchainBit    = std::max({None, DepthBit, ColorBit, TextureBit, StorageBit}) << 1,

    // Never use directly:
    Attachment      = ColorBit | DepthBit,
    DepthTexture    = TextureBit | DepthBit
};

inline constexpr ImageUsage operator|(ImageUsage l, ImageUsage r) {
    return ImageUsage(u32(l) | u32(r));
}

inline constexpr ImageUsage operator|(ImageUsage l, VkImageUsageFlagBits r) {
    return ImageUsage(u32(l) | u32(r));
}

inline constexpr ImageUsage operator&(ImageUsage l, ImageUsage r)  {
    return ImageUsage(u32(l) & u32(r));
}

inline constexpr ImageUsage operator~(ImageUsage l) {
    return ImageUsage(~u32(l));
}

inline constexpr bool is_copy_usage(ImageUsage usage) {
    return (usage & (ImageUsage::TransferDstBit | ImageUsage::TransferSrcBit)) != ImageUsage::None;
}

inline constexpr bool is_attachment_usage(ImageUsage usage) {
    return (usage & ImageUsage::Attachment) != ImageUsage::None;
}

inline constexpr bool is_storage_usage(ImageUsage usage) {
    return (usage & ImageUsage::StorageBit) != ImageUsage::None;
}

inline constexpr bool is_texture_usage(ImageUsage usage) {
    return (usage & ImageUsage::TextureBit) != ImageUsage::None;
}

VkImageLayout vk_image_layout(ImageUsage usage);



enum class ImageType : u32 {
    TwoD    = VK_IMAGE_VIEW_TYPE_2D,
    Layered = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
    Cube    = VK_IMAGE_VIEW_TYPE_CUBE,
    ThreeD  = VK_IMAGE_VIEW_TYPE_3D,
};

}

#endif // YAVE_GRAPHICS_IMAGES_IMAGEUSAGE_H

