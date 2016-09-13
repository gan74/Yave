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



enum class ImageUsageBits {
	None = 0,
	TextureBit = vk::ImageUsageFlagBits::eSampled,
	DepthBit = vk::ImageUsageFlagBits::eDepthStencilAttachment,
	ColorBit = vk::ImageUsageFlagBits::eColorAttachment,

	SwapchainBit = detail::max(None, DepthBit, ColorBit) << 1

};

constexpr ImageUsageBits operator|(ImageUsageBits l, ImageUsageBits r) { return ImageUsageBits(uenum(l) | uenum(r)); }
constexpr ImageUsageBits operator&(ImageUsageBits l, ImageUsageBits r)  { return ImageUsageBits(uenum(l) & uenum(r)); }

inline ImageUsageBits operator|(ImageUsageBits l, vk::ImageUsageFlags r) { return l | ImageUsageBits(uenum(r)); }
//inline ImageUsageBits operator|(ImageUsageBits l, vk::ImageUsageFlagBits r) { return l | r; }



class ImageUsage {
	public:
		ImageUsage(ImageUsageBits bits) : _usage(vk::ImageUsageFlagBits(bits)) {
		}

		vk::ImageUsageFlags get_vk_image_usage() const;
		vk::ImageLayout get_vk_image_layout() const;
		vk::AccessFlags get_vk_access_flags() const;


		operator ImageUsageBits() const {
			const auto& ref = _usage;
			return reinterpret_cast<const ImageUsageBits&>(ref);
		}

	private:
		vk::ImageUsageFlags _usage;
};


}

#endif // YAVE_IMAGE_IMAGEUSAGE_H
