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
#include "ImageData.h"

namespace n {
namespace graphics {

class Image : public assets::Asset<ImageData>
{
	public:
		Image() {
		}

		Image(const Image &image) : Image((assets::Asset<ImageData>)image) {
		}

		Image(ImageData *i) : assets::Asset<ImageData>(std::move(i)) {
		}

		Image(const math::Vec2ui &s, ImageFormat f = ImageFormat::RGBA8, void *c = 0) : Image(new ImageData(s, f, c)) {
		}

		Image(const assets::Asset<ImageData> &t) : assets::Asset<ImageData>(t) {
		}

		math::Vec2ui getSize() const {
			const ImageData *in = getInternal();
			if(!in) {
				return math::Vec2ui(0);
			}
			return in->size;
		}

		const void *data() const {
			const ImageData *in = getInternal();
			if(!in) {
				return 0;
			}
			return in->data;
		}

		ImageFormat getFormat() const {
			const ImageData *in = getInternal();
			if(!in)  {
				return ImageFormat(ImageFormat::None);
			}
			return in->format;
		}

		template<typename T = float>
		const Color<T> getPixel(const math::Vec2ui &pos) const {
			const ImageData *in = getInternal();
			uint offset = pos.x() * in->size.y() + pos.y();
			byte *colorData = (byte *)in->data;
			return Color<T>(colorData + offset * in->format.bytesPerPixel(), in->format);
		}

		bool operator==(const Image &i) const {
			return Asset<ImageData>::operator==(i);
		}

		bool operator!=(const Image &i) const {
			return !operator==(i);
		}

		bool operator<(const Image &i) const {
			return operator->() < i.operator->();
		}


	private:

		const ImageData *getInternal() const {
			return isValid() ? this->operator->() : (const ImageData *)0;
		}

};

}
}

#endif // N_GRAPHICS_IMAGE_H
