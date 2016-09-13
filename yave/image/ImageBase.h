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
#ifndef YAVE_IMAGE_IMAGEBASE_H
#define YAVE_IMAGE_IMAGEBASE_H

#include "ImageUsage.h"

#include <yave/DeviceLinked.h>

namespace yave {

class ImageBase : NonCopyable, public DeviceLinked {
	public:
		vk::Image get_vk_image() const;
		vk::DeviceMemory get_vk_device_memory() const;
		vk::ImageView get_vk_view() const;

		const math::Vec2ui& size() const;
		ImageFormat get_format() const;

		usize byte_size() const;

		~ImageBase();

	protected:
		ImageBase() = default;
		ImageBase(DevicePtr dptr, ImageFormat format, ImageUsage usage, const math::Vec2ui& size, const void* data = nullptr);

		void swap(ImageBase& other);

		math::Vec2ui _size;
		ImageFormat _format;
		vk::Image _image;
		vk::DeviceMemory _memory;
		vk::ImageView _view;
};


}

#endif // YAVE_IMAGE_IMAGEBASE_H
