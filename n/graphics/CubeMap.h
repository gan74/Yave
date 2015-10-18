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

#ifndef N_GRAPHICS_CUBEMAP
#define N_GRAPHICS_CUBEMAP

#include "Texture.h"
#include "TextureBase.h"

/*namespace n {
namespace graphics {

class CubeMap : public TextureBase<TextureCube>
{
	public:
		CubeMap(const Texture &top, const Texture &bottom, const Texture &right, const Texture &left, const Texture &front, const Texture &back);
		CubeMap(const Texture &tex) : CubeMap(tex, tex, tex, tex, tex, tex) {
		}

		bool isNull() const {
			return !getHandle();
		}

		bool isComplete() const {
			for(uint i = 0; i != 6; i++) {
				if(sides[i].isNull()) {
					return false;
				}
			}
			return true;
		}

	private:
		friend class ShaderProgram;
		friend class internal::TextureBinding;

		void upload() const;

		void prepare(bool sync = false) const;

		Texture sides[6];
};

}
}*/

#endif // N_GRAPHICS_CUBEMAP

