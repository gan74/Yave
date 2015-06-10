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

#ifndef N_GRAPHICS_CAMERA
#define N_GRAPHICS_CAMERA

#include "Transformable.h"
#include <n/math/Volume.h>
#include <n/utils.h>

namespace n {
namespace graphics {

class Camera : public Transformable, public math::Volume<>
{
	public:
		enum RotationAxis
		{
			Side,
			Up
		};

		Camera() : Transformable(), math::Volume<>() {
		}

		const math::Matrix4<> &getProjectionMatrix() const {
			return proj;
		}

		const math::Matrix4<> &getViewMatrix() const {
			return view;
		}

	protected:
		math::Matrix4<> view;
		math::Matrix4<> proj;
};


}
}

#endif // N_GRAPHICS_CAMERA

