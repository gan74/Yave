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

#include "TransientMipView.h"

namespace yave {

static VkHandle<VkImageView> create_mip_view(VkImage image, ImageFormat format, u32 mip) {
    VkImageViewCreateInfo create_info = vk_struct();
    {
        create_info.image = image;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        create_info.format = format.vk_format();
        create_info.subresourceRange.aspectMask = format.vk_aspect();
        create_info.subresourceRange.layerCount = 6;
        create_info.subresourceRange.baseMipLevel = mip;
        create_info.subresourceRange.levelCount = 1;
    }

    VkHandle<VkImageView> view;
    vk_check(vkCreateImageView(vk_device(), &create_info, vk_allocation_callbacks(), view.get_ptr_for_init()));
    return view;
}

TransientMipViewContainer::TransientMipViewContainer(const ImageBase& base, u32 mip) : _view_handle(create_mip_view(base.vk_image(), base.format(), mip)) {
}

}
