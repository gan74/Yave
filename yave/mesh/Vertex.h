/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#ifndef YAVE_VERTEX_H
#define YAVE_VERTEX_H

#include <yave/yave.h>
#include <y/math/Vec.h>

namespace yave {

struct Vertex {
	math::Vec3 position;
	math::Vec3 normal;
	math::Vec2 uv;
};

using IndexedTriangle = std::array<u32, 3>;

static_assert(std::is_trivially_copyable<math::Vec4>::value, "Vec4 should be trivially copyable");
static_assert(std::is_trivially_copyable<Vertex>::value, "Vertex should be trivially copyable");
}

#endif // YAVE_VERTEX_H
