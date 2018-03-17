/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#include <y/math/Transform.h>

namespace yave {

class Transformable {

	public:
		auto& transform() {
			return _transform;
		}

		const auto& transform() const {
			return _transform;
		}


		const auto& position() const {
			return _transform.position();
		}

		auto& position() {
			return _transform.position();
		}

		const auto& forward() const {
			return _transform.forward();
		}

		const auto& left() const {
			return _transform.left();
		}

		const auto& up() const {
			return _transform.up();
		}


		float radius() const {
			return _radius;
		}

	protected:
		void set_radius(float r) {
			_radius = r;
		}

	private:
		math::Transform<> _transform;
		float _radius = 0.0f;
};

}

#endif // YAVE_OBJECTS_TRANSFORMABLE_H
