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

#ifndef N_GRAPHICS_TEXTUREBINDING
#define N_GRAPHICS_TEXTUREBINDING

#include "TextureBase.h"

namespace n {
namespace graphics {
namespace internal {

class TextureBinding
{
	public:
		TextureBinding();

		template<typename T>
		TextureBinding &operator=(const T &t) {
			t.synchronize();
			tex = t.data;
			return *this;
		}

		TextureBinding &operator=(TextureSampler smp) {
			sampler = smp;
			return *this;
		}

		void bind(uint slot) const;
		static void dirty();

	private:
		core::SmartPtr<TextureBase::Data> tex;
		TextureSampler sampler;
};

}
}
}
#endif // N_GRAPHICS_TEXTUREBINDING

