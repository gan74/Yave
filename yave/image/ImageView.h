/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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
#ifndef YAVE_IMAGE_IMAGEVIEW_H
#define YAVE_IMAGE_IMAGEVIEW_H

#include <yave/yave.h>

#include "Image.h"

namespace yave {

template<ImageUsage Usage>
class ImageView {

	public:
		template<ImageUsage ImgUsage, typename Enable = typename std::enable_if<(ImgUsage & Usage) == Usage>::type>
		ImageView(const Image<ImgUsage>& img) : _image(&img), _view(img.vk_view()) {
		}

		vk::ImageView vk_image_view() const {
			return _view;
		}

		const ImageBase& image() const {
			return *_image;
		}

	private:
		const ImageBase* _image;
		vk::ImageView _view;
};

using TextureView = ImageView<ImageUsage::Texture>;
using DepthAttachmentView = ImageView<ImageUsage::Depth>;
using ColorAttachmentView = ImageView<ImageUsage::Color>;

}

#endif // YAVE_IMAGE_IMAGEVIEW_H
