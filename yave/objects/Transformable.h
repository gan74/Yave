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

		const math::Vec3& position() const {
			return transform()[3].to<3>();
		}

		const math::Vec3& forward() const {
			return transform()[0].to<3>();
		}

		const math::Vec3& right() const {
			return transform()[1].to<3>();
		}

		const math::Vec3& up() const {
			return transform()[2].to<3>();
		}

		const Transform& transform() const {
			return _storage;
		}


		Transform& transform() {
			return _storage;
		}

		math::Vec3& position() {
			return transform()[3].to<3>();
		}

		void set_basis(const math::Vec3& forward, const math::Vec3& up) {
			auto right = forward.cross(up);
			transform()[0].to<3>() = forward;
			transform()[1].to<3>() = right;
			transform()[2].to<3>() = up;
		}


		float radius() const {
			return _radius;
		}

	protected:
		void set_radius(float r) {
			_radius = r;
		}


	private:
		Transform _storage;

		float _radius = 0.0f;
};

}

#endif // YAVE_OBJECTS_TRANSFORMABLE_H
