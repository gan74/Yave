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
#ifndef YAVE_OBJECTS_TRANSFORMABLE_H
#define YAVE_OBJECTS_TRANSFORMABLE_H

#include <yave/yave.h>
#include <y/math/Matrix.h>

namespace yave {

class Transformable : NonCopyable {

	public:
		using Transform = math::Matrix4<>;

		Transformable() : _transform(nullptr) /*_transform(&_storage), _storage(math::identity())*/ {
		}

		Transformable(Transformable&& other) : Transformable() {
			swap(other);
		}

		Transformable& operator=(Transformable&& other) {
			swap(other);
			return *this;
		}

		math::Vec3 position() const {
			return math::Vec3(transform()[3][0], transform()[3][1], transform()[3][2]);
		}

		void set_position(const math::Vec3& p) {
			transform()[3][0] = p.x();
			transform()[3][1] = p.y();
			transform()[3][2] = p.z();
		}

		const Transform& transform() const {
			return *_transform;
		}

		Transform& transform() {
			return *_transform;
		}

	protected:
		void set_storage(Transform* s) {
			/*if(!s) {
				s = &_storage;
			}
			*s = *_transform;*/
			_transform = s;
		}

		void swap(Transformable& other) {
			//std::swap(_storage, other._storage);
			std::swap(_transform, other._transform);
			/*if(_transform == &other._storage) {
				_transform = &_storage;
			}
			if(other._transform == &_storage) {
				other._transform = &other._storage;
			}*/
		}

	private:
		Transform* _transform;
		//Transform _storage;
};

}

#endif // YAVE_OBJECTS_TRANSFORMABLE_H
