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
#ifndef YAVE_IMAGE_IMAGEFORMAT_H
#define YAVE_IMAGE_IMAGEFORMAT_H

#include <yave/yave.h>

namespace yave {

class ImageFormat {
	public:
		ImageFormat(vk::Format format = vk::Format::eUndefined);

		vk::Format vk_format() const;
		vk::ImageAspectFlags vk_aspect() const;

		usize bpp() const;

	private:
		vk::Format _format;
};

}

#endif // YAVE_IMAGE_IMAGEFORMAT_H
