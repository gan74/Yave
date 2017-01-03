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
#ifndef YAVE_IMAGE_IMAGEBASE_H
#define YAVE_IMAGE_IMAGEBASE_H

#include "ImageUsage.h"

#include <yave/DeviceLinked.h>

namespace yave {

class ImageBase : NonCopyable, public DeviceLinked {
	public:
		vk::Image vk_image() const;
		vk::DeviceMemory vk_device_memory() const;
		vk::ImageView vk_view() const;

		const math::Vec2ui& size() const;
		ImageFormat format() const;
		ImageUsage usage() const;

		usize byte_size() const;

		~ImageBase();

	protected:
		ImageBase() = default;
		ImageBase(DevicePtr dptr, ImageFormat fmt, ImageUsage usage, const math::Vec2ui& size, const void* data = nullptr);

		void swap(ImageBase& other);

		math::Vec2ui _size;
		ImageFormat _format;
		ImageUsage _usage;

		vk::Image _image;
		vk::DeviceMemory _memory;
		vk::ImageView _view;
};


}

#endif // YAVE_IMAGE_IMAGEBASE_H
