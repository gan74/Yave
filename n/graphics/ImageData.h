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

#ifndef N_GRAPHICS_IMAGEDATA
#define N_GRAPHICS_IMAGEDATA

#include "Color.h"

namespace n {
namespace graphics {

struct ImageData : core::NonCopyable
{
	ImageData(const math::Vec2ui &s, ImageFormat f = ImageFormat::RGBA8, const void *c = 0, bool flip = true) : format(f), size(s), data(c ? new byte[s.mul() * f.bytesPerPixel()] : 0) {
		if(c) {
			if(flip) {
				uint bpp = format.bytesPerPixel();
				for(uint i = 0; i != size.y(); i++) {
					byte *dat = (byte *)data;
					const byte *bc = (const byte *)c;
					uint j = size.y() - (i + 1);
					memcpy(dat + i * size.x() * bpp, bc + j * size.x() * bpp, size.x() * bpp);
				}
			} else {
				memcpy((void *)data, c, s.mul() * format.bytesPerPixel());
			}
		}
	}

	~ImageData() {
		delete[] (byte *)data;
	}

	const ImageFormat format;
	const math::Vec2ui size;
	const void *data;

};

}
}

#endif // N_GRAPHICS_IMAGEDATA

