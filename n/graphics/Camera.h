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

template<typename T = float>
class Camera : public Transformable<T>, public math::Volume<T>
{
	protected:
		using Transformable<T>::transform; // WHY ?
		using Transformable<T>::radius; // WHY ?

	public:
		enum RotationAxis
		{
			Side,
			Up
		};

		Camera() : Transformable<T>(), math::Volume<T>() {
		}

		const math::Matrix4<T> &getProjectionMatrix() const {
			return proj;
		}

		const math::Matrix4<T> &getViewMatrix() const {
			return view;
		}

	protected:
		math::Matrix4<T> view;
		math::Matrix4<T> proj;
};


}
}

#endif // N_GRAPHICS_CAMERA

