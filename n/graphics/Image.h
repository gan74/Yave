/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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

#ifndef N_GRAPHICS_IMAGE_H
#define N_GRAPHICS_IMAGE_H

#include <n/assets/Asset.h>
#include "Color.h"

namespace n {
namespace graphics {

namespace internal {
struct Image : core::NonCopyable
{
	Image(const math::Vec2ui &s, void *c, ImageFormat f = ImageFormat::R8G8B8A8) : format(f), size(s), data(new byte[s.mul() * format.bytePerPixel()]) {
		if(c) {
			memcpy((void *)data, c, s.mul() * format.bytePerPixel());
		}
	}

	const ImageFormat format;
	const math::Vec2ui size;
	const void *data;

};
}



class Image : private assets::Asset<internal::Image>
{
	friend class ImageLoader;
	public:
		Image() {
		}

		Image(const Image &image) : Image((assets::Asset<internal::Image>)image) {
		}

		Image(internal::Image *i) : assets::Asset<internal::Image>(std::move(i)) {
		}

		Image(const math::Vec2ui &s, void *c = 0, ImageFormat f = ImageFormat::R8G8B8A8) : Image(new internal::Image(s, c, f)) {
		}

		math::Vec2ui getSize() const {
			const internal::Image *in = getInternal();
			if(!in) {
				return math::Vec2ui(0);
			}
			return in->size;
		}

		bool isValid() const {
			return assets::Asset<internal::Image>::isValid();
		}

		bool isNull() const {
			return assets::Asset<internal::Image>::isNull();
		}

		const void *data() const {
			const internal::Image *in = getInternal();
			if(!in) {
				return 0;
			}
			return in->data;
		}

		ImageFormat getFormat() const {
			const internal::Image *in = getInternal();
			if(!in)  {
				return ImageFormat(ImageFormat::None);
			}
			return in->format;
		}

		template<typename T = float>
		const Color<T> getPixel(const math::Vec2ui &pos) const {
			const internal::Image *in = getInternal();
			uint offset = pos.x() * in->size.y() + pos.y();
			return Color<T>((void *)(((byte *)in->data) + offset * in->format.bytePerPixel()), in->format);
		}

		bool operator==(const Image &i) const {
			return Asset<internal::Image>::operator==(i);
		}

		bool operator!=(const Image &i) const {
			return !operator==(i);
		}


	private:
		Image(const assets::Asset<internal::Image> &t) : assets::Asset<internal::Image>(t) {
		}

		const internal::Image *getInternal() const {
			return isValid() ? this->operator->() : (const internal::Image *)0;
		}

};

}
}

#endif // N_GRAPHICS_IMAGE_H
