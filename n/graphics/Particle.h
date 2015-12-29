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

#ifndef N_GRAPHICS_PARTICLE
#define N_GRAPHICS_PARTICLE

#include <n/math/Vec.h>

namespace n {
namespace graphics {

struct Particle
{
	math::Vec4 position;
	math::Vec4 velocity;
	float life;
	float dLife;
	int material;
	float padding;


	static core::String toShader() {
		return
			"struct Particle {"
				"vec4 position;"
				"vec4 velocity;"
				"float life;"
				"float dLife;"
				"int material;"
				"float padding;"
			"};";
	}
};

}
}

#endif // N_GRAPHICS_PARTICLE

