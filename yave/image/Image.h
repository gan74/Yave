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
#ifndef YAVE_IMAGE_IMAGE_H
#define YAVE_IMAGE_IMAGE_H

#include <yave/yave.h>

#include "ImageBase.h"

namespace yave {

template<ImageUsage Usage>
class Image : public ImageBase {
	public:
		Image() = default;

		Image(DevicePtr dptr, ImageFormat format, const math::Vec2ui& image_size) : ImageBase(dptr, ImageFormat(format), Usage, image_size) {
			static_assert(is_attachment_usage(Usage), "Texture images must be initilized.");
		}

		Image(DevicePtr dptr, ImageFormat format, const math::Vec2ui& image_size, const u8* data) : ImageBase(dptr, ImageFormat(format), Usage, image_size, data) {
		}

		Image(Image&& other) {
			swap(other);
		}

		Image& operator=(Image&& other) {
			swap(other);
			return *this;
		}

};

using Texture = Image<ImageUsage::TextureBit>;
using DepthAttachment = Image<ImageUsage::DepthBit>;
using ColorAttachment = Image<ImageUsage::ColorBit>;
using DepthTextureAttachment = Image<ImageUsage::DepthBit | ImageUsage::TextureBit>;
using ColorTextureAttachment = Image<ImageUsage::ColorBit | ImageUsage::TextureBit>;

}

#endif // YAVE_IMAGE_IMAGE_H
