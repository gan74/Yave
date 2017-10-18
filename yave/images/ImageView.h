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
#ifndef YAVE_IMAGES_IMAGEVIEW_H
#define YAVE_IMAGES_IMAGEVIEW_H

#include <yave/yave.h>

#include "Image.h"

namespace yave {

template<ImageUsage Usage, ImageType Type = ImageType::TwoD>
class ImageView {

	static constexpr bool is_compatible(ImageUsage u) {
		return (uenum(Usage) & uenum(u)) == uenum(Usage);
	}

	public:
		ImageView() = default;

		template<ImageUsage U, typename = std::enable_if_t<is_compatible(U)>>
		ImageView(const Image<U, Type>& img) : ImageView(&img, img.vk_view()) {
		}

		template<ImageUsage U, typename = std::enable_if_t<is_compatible(U)>>
		ImageView(const ImageView<U, Type>& img) : ImageView(img._image, img._view) {
		}

		vk::ImageView vk_view() const {
			return _view;
		}

		DevicePtr device() const {
			return _image->device();
		}

		const ImageBase& image() const {
			return *_image;
		}

		const math::Vec2ui& size() const {
			return _image->size();
		}

		bool operator==(const ImageView& other) const {
			return _image == other._image && _view == other._view;
		}

		bool operator!=(const ImageView& other) const {
			return !operator==(other);
		}

	protected:
		ImageView(const ImageBase* image, vk::ImageView view) : _image(image), _view(view) {
		}

	private:
		template<ImageUsage U, ImageType T>
		friend class ImageView;

		const ImageBase* _image = nullptr;
		vk::ImageView _view;
};

using TextureView = ImageView<ImageUsage::TextureBit>;
using StorageView = ImageView<ImageUsage::StorageBit>;
using DepthAttachmentView = ImageView<ImageUsage::DepthBit>;
using ColorAttachmentView = ImageView<ImageUsage::ColorBit>;
using DepthTextureAttachmentView = ImageView<ImageUsage::DepthBit | ImageUsage::TextureBit>;
using ColorTextureAttachmentView = ImageView<ImageUsage::ColorBit | ImageUsage::TextureBit>;

using CubemapView = ImageView<ImageUsage::TextureBit, ImageType::Cube>;
using CubemapStorageView = ImageView<ImageUsage::StorageBit, ImageType::Cube>;

}

#endif // YAVE_IMAGES_IMAGEVIEW_H
