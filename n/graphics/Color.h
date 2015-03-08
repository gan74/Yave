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

#ifndef N_GRAPHICS_COLOR_H
#define N_GRAPHICS_COLOR_H

#include <n/math/Vec.h>
#include <n/utils.h>

namespace n {
namespace graphics {

struct ImageFormat
{
	enum Format
	{
		None,
		R8G8B8A8,
		R16G16B16A16,
		R8G8B8,
		R10G10B10A2,
		F32,
		Depth32
	};

	enum Channel
	{
		Red,
		Green,
		Blue,
		Alpha,
		Float
	};


	struct R10G10B10A2_t
	{
		uint32 r : 10;
		uint32 g : 10;
		uint32 b : 10;
		uint32 a : 2;
	};

	struct R8G8B8A8_t
	{
		uint32 r : 8;
		uint32 g : 8;
		uint32 b : 8;
		uint32 a : 8;
	};

	ImageFormat(Format fr) : format(fr) {
	}

	operator Format() const {
		return format;
	}

	uint bytePerPixel() const {
		return format == None ? 0 : 4;
	}

	bool hasAlpha() const {
		switch(format) {
			case R8G8B8:
			case F32:
			case Depth32:
				return false;

			default:
				return true;
		}
	}

	uint getBits(Channel ch) const {
		switch(format) {
			case R8G8B8A8:
				return 8;
			case R16G16B16A16:
				return 16;
			case R8G8B8:
				return ch == Alpha ? 0 : 8;
			case R10G10B10A2:
				return ch == Alpha ? 2 : 10;
			case F32:
			case Depth32:
				return 32;
			default:
				return 0;
		}
	}

	uint64 rawData(const void *cdt, Channel ch) const {
		if(!hasAlpha() && ch == Alpha) {
			return 0;
		}
		switch(format) {
			case R8G8B8:
			case R8G8B8A8:
				return ((byte *)cdt)[ch];

			case R16G16B16A16:
				return ((uint16 *)cdt)[ch];

			case R10G10B10A2: {
				R10G10B10A2_t data = *(R10G10B10A2_t *)cdt;
				switch(ch) {
					case Red:
						return data.r;
					case Green:
						return data.g;
					case Blue:
						return data.b;
					case Alpha:
						return data.a;
					default:
						return 0;
				}
			} break;

			case Depth32:
			case F32:
				return (uint64)(((double)*(float *)cdt) * ((uint32)-1));

			default:
				return 0;
		}
		return 0;
	}

	double normalizedData(const void *data, Channel ch) const {
		uint64 d = rawData(data, ch);
		return d ? double(d) / double((uint64(1) << getBits(ch)) - 1) : 0.0;
	}

	private:
		const Format format;
};

template<typename T = float>
class Color : public math::Vec<4, T>
{
	public:
		template<typename U>
		Color(const U &t, ImageFormat format) : math::Vec<4, T>(math::normalizedConversion<T>(format.normalizedData(&t, ImageFormat::Red)),
																math::normalizedConversion<T>(format.normalizedData(&t, ImageFormat::Green)),
																math::normalizedConversion<T>(format.normalizedData(&t, ImageFormat::Blue)),
																math::normalizedConversion<T>(format.normalizedData(&t, ImageFormat::Alpha))) {
		}

		template<typename... Args>
		Color(Args... args) : math::Vec<4, T>(args...) {
		}
};

}
}
#endif // N_GRAPHICS_COLOR_H
