/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
	Texture = int(vk::ImageUsageFlagBits::eSampled),
	Depth = int(vk::ImageUsageFlagBits::eDepthStencilAttachment),
	Color = int(vk::ImageUsageFlagBits::eColorAttachment),

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

vk::ImageLayout vk_image_layout(ImageUsage usage);
vk::AccessFlags vk_access_flags(ImageUsage usage);

}

#endif // YAVE_IMAGE_IMAGEUSAGE_H
