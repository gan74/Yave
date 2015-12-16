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
#include "TextureArray.h"

namespace n {
namespace graphics {

class CubeMap : public TextureBase<TextureCube>
{

	public:
		struct Cube
		{
			Texture top;
			Texture bottom;
			Texture right;
			Texture left;
			Texture front;
			Texture back;
		};


		CubeMap(const TextureArray &array);
		CubeMap(const Cube &sides);
		CubeMap(Texture top, Texture bottom, Texture right, Texture left, Texture front, Texture back) : CubeMap(Cube{top, bottom, right, left, front, back}) {
		}


		bool synchronize(bool sync = false) const;

	private:
		void upload() const;
		bool syncCube(bool sync) const;

		mutable core::SmartPtr<TextureArray> texArray;
		mutable core::SmartPtr<Cube> cube;


};

}
}

#endif // N_GRAPHICS_CUBEMAP

