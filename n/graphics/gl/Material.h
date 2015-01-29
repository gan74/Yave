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

#ifndef N_GRAPHICS_GL_MATERIAL
#define N_GRAPHICS_GL_MATERIAL

#include <n/math/Vec.h>
#include "Texture.h"
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

template<typename T = float>
class Material
{
	public:
		Material() : index(cache.size()), roughtness(1), specular(0) { // not tsafe
			cache.append(this);
		}

		void setColor(const Color<T> &c) {
			color = c;
			updateCache();
		}

		bool operator<(const Material<T> &m) const {
			return index < m.index;
		}

	private:
		static core::Array<Material<T> *> cache;

		bool cacheCompare(const Material<T> &m) const {
			if(color != m.color) {
				return color < m.color;
			}
			if(roughtness != m.roughtness) {
				return roughtness < m.roughtness;
			}
			if(specular != m.specular) {
				return specular < m.specular;
			}
			return false;
		}

		static void updateCache() {
			cache.sort([](const Material *a, const Material *b) { return a->cacheCompare(*b); });
			for(uint i = 0; i != cache.size(); i++) {
				cache[i]->index = i;
			}
		}

		uint index;

		Color<T> color;
		T roughtness;
		T specular;
		Texture diffuse;
};

template<typename T>
core::Array<Material<T> *> Material<T>::cache = core::Array<Material<T> *>();

}
}
}

#endif

#endif // N_GRAPHICS_GL_MATERIAL

