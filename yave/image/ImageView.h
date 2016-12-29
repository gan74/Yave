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
