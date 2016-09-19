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
#ifndef YAVE_VIEWPORT_H
#define YAVE_VIEWPORT_H

#include "yave.h"
#include <y/math/Vec.h>

namespace yave {

struct Viewport {
	math::Vec2 extent;
	math::Vec2 offset;
	math::Vec2 depth;


	Viewport(const math::Vec2& size = math::Vec2()) : extent(size), depth(0.0f, 1.0f) {
	}
};

}

#endif // YAVE_VIEWPORT_H
