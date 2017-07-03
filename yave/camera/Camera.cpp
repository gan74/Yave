/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include "Camera.h"

namespace yave {

static math::Vec3 extract_position(const math::Matrix4<>& view) {
	math::Vec3 pos;
	for(usize i = 0; i != 3; i++) {
		auto v = view.row(i);
		pos -= v.to<3>() * v.w();
	}
	return pos;
}

static math::Vec3 extract_forward(const math::Matrix4<>& view) {
	return -view.row(2).to<3>();
}

static std::array<Plane, 6> extract_frustum(const math::Matrix4<>& viewproj) {
	auto x = viewproj.row(0);
	auto y = viewproj.row(1);
	auto z = viewproj.row(2);
	auto w = viewproj.row(3);
	return {{
			(w + x).normalized(),
			(w - x).normalized(),
			(w + y).normalized(),
			(w - y).normalized(),
			(w + z).normalized(),
			(w - z).normalized()
		}};
}

Camera::Camera() {
	float ratio = 4.0f / 3.0f;
	set_proj(math::perspective(math::to_rad(45), ratio, 0.001f, 10.f));
	set_view(math::look_at({2.0f, 0.0f, 0.0f}, math::Vec3{}));
}

void Camera::set_view(const math::Matrix4<>& view) {
	_view = view;
	_up_to_date = false;
}

void Camera::set_proj(const math::Matrix4<>& proj) {
	_proj = proj;
	_up_to_date = false;
}

void Camera::update_viewproj() const {
	if(!_up_to_date) {
		_viewproj = _proj * _view;
		_up_to_date = true;
	}
}

const math::Matrix4<>& Camera::view_matrix() const {
	return _view;
}

const math::Matrix4<>& Camera::proj_matrix() const {
	return _proj;
}

const math::Matrix4<>& Camera::viewproj_matrix() const {
	update_viewproj();
	return _viewproj;
}

math::Matrix4<> Camera::inverse_matrix() const {
	return (viewproj_matrix()).inverse();
}

math::Vec3 Camera::position() const {
	return extract_position(_view);
}

Frustum Camera::frustum() const {
	return extract_frustum(viewproj_matrix());
}

Camera::operator uniform::Camera() const {
	return uniform::Camera {
			inverse_matrix(),
			position(),
			0,
			extract_forward(_view),
			0
		};
}

}
