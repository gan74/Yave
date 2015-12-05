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

#ifndef N_GRAPHICS_TEXTUREARRAY
#define N_GRAPHICS_TEXTUREARRAY

#include "TextureBase.h"
#include "Texture.h"

namespace n {
namespace graphics {

class TextureArray : public TextureBase<Texture2DArray>
{

	public:
		TextureArray(const math::Vec3ui &si, ImageFormat format);
		TextureArray();
		~TextureArray();

		const math::Vec3ui &getSize() const;

		Texture getTexture(uint index) const;

		bool isNull() const {
			return !getHandle();
		}

		bool isValid() {
			return size.mul() != 0;
		}

		bool synchronize(bool immediate = false);

	private:
		math::Vec3ui size;
		ImageFormat imgFormat;

		bool isMipCapable() const {
			return getSize().sub(2).max() / (getSize().sub(2).min() + 1) < sqrt(2);
		}

		bool prepare(bool sync = false) const;
		void upload() const;

		mutable Texture *textures;


};

}
}

#endif // N_GRAPHICS_TEXTUREARRAY

