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
			R32FG32FB32FA32F,
			R16G16,
			F32,
			Depth32
		};

		enum Channel
		{
			Red = 0,
			Green = 1,
			Blue = 2,
			Alpha = 3,
			Float = 0
		};

	private:
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
	public:

		ImageFormat(Format fr) : format(fr) {
		}

		operator Format() const {
			return format;
		}

		uint bytesPerPixel() const {
			switch(format) {
				case None:
					return 0;

				case R16G16B16A16:
					return 8;

				case R32FG32FB32FA32F:
					return 16;

				default:
					return 4;
			}
		}

		bool hasAlpha() const {
			switch(format) {
				case None:
				case R8G8B8:
				case R16G16:
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
				case R16G16:
					return ch < Blue ? 16 : 0;
				case R16G16B16A16:
					return 16;
				case R8G8B8:
					return ch == Alpha ? 0 : 8;
				case R10G10B10A2:
					return ch == Alpha ? 2 : 10;
				case R32FG32FB32FA32F:
				case F32:
				case Depth32:
					return 32;
				default:
					return 0;
			}
		}

		double normalizedData(const void *cdt, Channel ch) const {
			switch(format) {
				case R8G8B8:
				case R8G8B8A8:
					return norm(((uint8 *)cdt)[ch]);

				case R16G16B16A16:
					return norm(((uint16 *)cdt)[ch]);

				case R10G10B10A2: {
					R10G10B10A2_t data = *(R10G10B10A2_t *)cdt;
					switch(ch) {
						case Red:
							return norm(data.r, 10);
						case Green:
							return norm(data.g, 10);
						case Blue:
							return norm(data.b, 10);
						case Alpha:
							return norm(data.a, 2);
						default:
							return 0;
					}
				}

				case R16G16:
					return norm(((uint16 *)cdt)[ch]);

				case R32FG32FB32FA32F:
					return ((const float *)cdt)[ch];

				case Depth32:
				case F32:
					return ((const float *)cdt)[0];

				default:
					return 0;
			}
			return 0;
		}

	private:
		static double norm(uint8 b) {
			uint8 m = ~0;
			return double(b) / double(m);
		}

		static double norm(uint16 b) {
			uint8 m = ~0;
			return double(b) / double(m);
		}

		static double norm(uint32 b) {
			uint32 m = ~0;
			return double(b) / double(m);
		}

		static double norm(uint32 b, uint bits) {
			uint32 m = ~((~0 >> bits) << bits);
			return double(b) / double(m);
		}

		const Format format;
};

template<typename T = float>
class Color : public math::Vec<4, T>
{
	public:
		template<typename U>
		Color(const U *t, ImageFormat format) : math::Vec<4, T>(math::normalizedConversion<T>(format.normalizedData(t, ImageFormat::Red)),
																math::normalizedConversion<T>(format.normalizedData(t, ImageFormat::Green)),
																math::normalizedConversion<T>(format.normalizedData(t, ImageFormat::Blue)),
																math::normalizedConversion<T>(format.normalizedData(t, ImageFormat::Alpha))) {
		}

		template<typename U>
		Color(U *t, ImageFormat format) : Color<T>((const U *)t, format) {
		}

		template<typename... Args>
		Color(Args... args) : math::Vec<4, T>(args...) {
		}
};

}
}
#endif // N_GRAPHICS_COLOR_H
