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

#include "ImageFormat.h"

namespace yave {

ImageFormat::ImageFormat(vk::Format format) : _format(format) {
}

vk::Format ImageFormat::get_vk_format() const {
	return _format;
}

usize ImageFormat::get_bpp() const {
	switch(_format) {
		case vk::Format::eR8G8B8A8Unorm:
			return 4;
		case vk::Format::eD32Sfloat:
			return 4;

		default:
			return fatal("Unsupported image format");
	}
}

vk::ImageAspectFlags ImageFormat::get_vk_aspect() const {
	switch(_format) {
		case vk::Format::eR8G8B8A8Unorm:
			return vk::ImageAspectFlagBits::eColor;

		case vk::Format::eD32Sfloat:
			return vk::ImageAspectFlagBits::eDepth;

		default:
			return fatal("Unsupported image format");
	}
}


}
