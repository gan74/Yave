/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef YAVE_IMAGE_IMAGEUSAGE_H
#define YAVE_IMAGE_IMAGEUSAGE_H

#include "ImageFormat.h"

namespace yave {

namespace detail {
template<typename... Args>
constexpr int max(Args... args) {
	int max = 0;
	// https://www.youtube.com/watch?v=nhk8pF_SlTk
	unused(std::initializer_list<int>{ (max = std::max(max, args))... });
	return max;
}
}

enum class ImageUsage {
    None = 0,
	Texture = uenum(vk::ImageUsageFlagBits::eSampled),
	Depth = uenum(vk::ImageUsageFlagBits::eDepthStencilAttachment),
	Color = uenum(vk::ImageUsageFlagBits::eColorAttachment),
	Storage = uenum(vk::ImageUsageFlagBits::eStorage),

	Attachment = Color | Depth,
	Swapchain = detail::max(None, Depth, Color, Texture) << 1

};

constexpr ImageUsage operator|(ImageUsage l, ImageUsage r) {
	return ImageUsage(uenum(l) | uenum(r));
}

inline ImageUsage operator|(ImageUsage l, vk::ImageUsageFlags r) {
	return l | ImageUsage(uenum(r));
}

constexpr ImageUsage operator&(ImageUsage l, ImageUsage r)  {
	return ImageUsage(uenum(l) & uenum(r));
}

constexpr ImageUsage operator~(ImageUsage l) {
	return ImageUsage(~uenum(l));
}

constexpr ImageUsage attachment_usage(ImageUsage usage) {
	return (usage & ImageUsage::Attachment);
}

constexpr ImageUsage shader_usage(ImageUsage usage) {
	return (usage & ~ImageUsage::Attachment);
}

constexpr bool is_attachment_usage(ImageUsage usage) {
	return attachment_usage(usage) != ImageUsage::None;
}

constexpr bool is_shader_usage(ImageUsage usage) {
	return shader_usage(usage) != ImageUsage::None;
}


//vk::ImageLayout vk_image_layout(ImageUsage usage);
vk::ImageLayout vk_attachment_image_layout(ImageUsage usage);
vk::ImageLayout vk_shader_image_layout(ImageUsage usage);

}

#endif // YAVE_IMAGE_IMAGEUSAGE_H
