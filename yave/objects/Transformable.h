/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef YAVE_OBJECTS_TRANSFORMABLE_H
#define YAVE_OBJECTS_TRANSFORMABLE_H

#include <yave/yave.h>
#include <y/math/Matrix.h>

namespace yave {

class Transformable {

	public:
		using Transform = math::Matrix4<>;

		Transformable() :  _storage(math::identity()) {
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
			//return *_transform;
			return _storage;
		}

		Transform& transform() {
			//return *_transform;
			return _storage;
		}

		float radius() const {
			return _radius;
		}

	protected:
		/*void set_storage(Transform* s) {
			_transform = s;
		}*/

		void set_radius(float r) {
			_radius = r;
		}


	private:
		//Transform* _transform = nullptr;
		Transform _storage;

		float _radius = 0.0f;
};

}

#endif // YAVE_OBJECTS_TRANSFORMABLE_H
