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
#ifndef YAVE_IMAGES_IMAGEBASE_H
#define YAVE_IMAGES_IMAGEBASE_H

#include "ImageUsage.h"
#include "ImageData.h"

#include <yave/device/DeviceLinked.h>
#include <yave/memory/DeviceMemory.h>

namespace yave {

class ImageBase : NonCopyable {

	public:
		DevicePtr device() const;

		vk::Image vk_image() const;
		vk::ImageView vk_view() const;

		const DeviceMemory& device_memory() const;

		const math::Vec2ui& size() const;
		usize mipmaps() const;

		usize layers() const;

		ImageFormat format() const;
		ImageUsage usage() const;

		~ImageBase();

	protected:
		ImageBase() = default;

		ImageBase(DevicePtr dptr, ImageFormat format, ImageUsage usage, const math::Vec2ui& size, ImageType type = ImageType::TwoD, usize layers = 1, usize mips = 1);
		ImageBase(DevicePtr dptr, ImageUsage usage, ImageType type, const ImageData& data);


		void swap(ImageBase& other);

		math::Vec2ui _size;
		u32 _layers = 1;
		u32 _mips = 1;

		ImageFormat _format;
		ImageUsage _usage;

		DeviceMemory _memory;

		vk::Image _image;
		vk::ImageView _view;
};

static_assert(is_safe_base<ImageBase>::value);

}

#endif // YAVE_IMAGES_IMAGEBASE_H
