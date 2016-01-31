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
			RGBA8,
			RGBA16,
			RGB8,
			RGB10A2,
			RGBA16F,
			RGBA32F,
			RG16,
			RG16F,
			RG32F,
			F32,
			R32F = F32,
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

				case RGBA16:
				case RGBA16F:
				case RG32F:
					return 8;

				case RGBA32F:
					return 16;

				default:
					return 4;
			}
		}

		bool hasAlpha() const {
			switch(format) {
				case None:
				case RGB8:
				case RG16:
				case RG16F:
				case RG32F:
				case F32:
				case Depth32:
					return false;

				default:
					return true;
			}
		}

		uint getBits(Channel ch) const {
			switch(format) {
				case RGBA8:
					return 8;
				case RG16:
					return ch < Blue ? 16 : 0;
				case RGBA16:
					return 16;
				case RGB8:
					return ch == Alpha ? 0 : 8;
				case RGB10A2:
					return ch == Alpha ? 2 : 10;
				case RG32F:
					return ch > Green ? 32 : 0;
				case RGBA32F:
				case F32:
				case Depth32:
					return 32;
				default:
					return fatal("Unsupported image format");
			}
		}

		double normalizedData(const void *cdt, Channel ch) const {
			switch(format) {
				case RGB8:
				case RGBA8:
					return norm(((uint8 *)cdt)[ch]);

				case RGBA16:
					return norm(((uint16 *)cdt)[ch]);

				case RGB10A2: {
					const R10G10B10A2_t data = *reinterpret_cast<const R10G10B10A2_t *>(cdt);
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

				case RG16:
					return norm(((uint16 *)cdt)[ch]);

				case RG32F:
				case RGBA32F:
					return ((const float *)cdt)[ch];

				case Depth32:
				case F32:
					return ((const float *)cdt)[0];

				default:
					return fatal("Unsupported image format");
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

		Format format;
};

enum BaseColor
{
	Black,
	White,
	Red,
	Green,
	Blue,
};

template<typename T = float>
class Color : public math::Vec<4, T>
{
	static math::Vec3 colors[Blue + 1];

	public:
		template<typename U>
		Color(const U *t, ImageFormat format) : math::Vec<4, T>(math::normalizedConversion<T>(format.normalizedData(t, ImageFormat::Red)),
																math::normalizedConversion<T>(format.normalizedData(t, ImageFormat::Green)),
																math::normalizedConversion<T>(format.normalizedData(t, ImageFormat::Blue)),
																math::normalizedConversion<T>(format.normalizedData(t, ImageFormat::Alpha))) {
		}

		Color(BaseColor c) : Color(colors[c], 1) {
		}

		template<typename U>
		Color(U *t, ImageFormat format) : Color<T>((const U *)t, format) {
		}

		template<typename... Args>
		Color(Args... args) : math::Vec<4, T>(args...) {
		}

		operator math::Vec<4, T>() const {
			return *this;
		}

		bool operator==(const Color<T> &c) const {
			return math::Vec<4, T>(*this) == math::Vec<4, T>(c);
		}

		bool operator!=(const Color<T> &c) const {
			return math::Vec<4, T>(*this) != math::Vec<4, T>(c);
		}

		bool operator<(const Color<T> &c) const {
			return math::Vec<4, T>(*this) < math::Vec<4, T>(c);
		}

		bool operator>(const Color<T> &c) const {
			return math::Vec<4, T>(*this) > math::Vec<4, T>(c);
		}

		T &r() {
			return this->x();
		}

		T &g() {
			return this->y();
		}

		T &b() {
			return this->z();
		}

		T &a() {
			return this->w();
		}

		const T &r()const  {
			return this->x();
		}

		const T &g()const  {
			return this->y();
		}

		const T &b()const  {
			return this->z();
		}

		const T &a()const  {
			return this->w();
		}

		Color<T> withLightness(T t) const {
			return math::Vec<4, T>(*this) * t / math::Vec<3, T>(0.2126, 0.7152, 0.0722).dot(this->sub(3));
		}
};

template<typename T>
math::Vec3 Color<T>::colors[] = {math::Vec3(1), math::Vec3(0), math::Vec3(1, 0, 0), math::Vec3(0, 1, 0), math::Vec3(0, 0, 1)};

}
}
#endif // N_GRAPHICS_COLOR_H
