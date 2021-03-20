/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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


static float extract_ratio(const math::Matrix4<>& proj) {
    const float f = proj[1][1];
    return 1.0f / (proj[0][0] / f);
}

static float extract_fov(const math::Matrix4<>& proj) {
    const float f = proj[1][1];
    return 2.0f * std::atan(1.0f / f);
}


static math::Vec3 extract_position(const math::Matrix4<>& view) {
    math::Vec3 pos;
    for(usize i = 0; i != 3; ++i) {
        const auto v = view.row(i);
        pos -= v.to<3>() * v.w();
    }
    return pos;
}

static math::Vec3 extract_forward(const math::Matrix4<>& view) {
    return -view.row(2).to<3>().normalized();
}

static math::Vec3 extract_right(const math::Matrix4<>& view) {
    return -view.row(0).to<3>().normalized();
}

static math::Vec3 extract_up(const math::Matrix4<>& view) {
    return -view.row(1).to<3>().normalized();
}


static Frustum extract_frustum(const math::Matrix4<>& view, const math::Matrix4<>& proj) {
    const float fov = extract_fov(proj);
    const float ratio = extract_ratio(proj);
    const math::Vec3 forward = extract_forward(view);
    const math::Vec3 up = extract_up(view);
    const math::Vec3 right = extract_right(view);
    const math::Vec3 pos = extract_position(view);

    const float half_fov = fov * 0.5f;
    const float half_fov_v = std::atan(std::tan(half_fov) * ratio);

    std::array<math::Vec3, 4> normals;
    {
        const float c = std::cos(half_fov);
        const float s = std::sin(half_fov);
        normals[0] = forward * s + up * c;
        normals[1] = forward * s - up * c;
    }
    {
        const float c = std::cos(half_fov_v);
        const float s = std::sin(half_fov_v);
        normals[2] = forward * s + right * c;
        normals[3] = forward * s - right * c;
    }

    return Frustum(normals, pos, forward);
}

Camera::Camera() {
    const float ratio = 4.0f / 3.0f;
    set_proj(math::perspective(math::to_rad(45.0f), ratio, 0.1f));
    set_view(math::look_at({2.0f, 0.0f, 0.0f}, math::Vec3{}, math::Vec3{0.0f, 0.0f, 1.0f}));
}

void Camera::set_view(const math::Matrix4<>& view) {
    _view = view;
    _dirty = true;
}

void Camera::set_proj(const math::Matrix4<>& proj) {
    _proj = proj;
    _dirty = true;
}

void Camera::update_viewproj() const {
    if(_dirty) {
        _viewproj = _proj * _view;
        _dirty = false;
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

float Camera::aspect_ratio() const {
    return extract_ratio(_proj);
}

float Camera::field_of_view() const {
    return extract_fov(_proj);
}

math::Vec3 Camera::position() const {
    return extract_position(_view);
}

math::Vec3 Camera::forward() const {
    return extract_forward(_view);
}

math::Vec3 Camera::right() const {
    return extract_right(_view);
}

math::Vec3 Camera::up() const {
    return extract_up(_view);
}

Frustum Camera::frustum() const {
    return extract_frustum(view_matrix(), proj_matrix());
}

Camera::operator uniform::Camera() const {
    uniform::Camera camera_data = {};
    camera_data.view_proj = viewproj_matrix();
    camera_data.inv_view_proj = inverse_matrix();
    camera_data.position = position();
    camera_data.forward = forward();
    camera_data.up = up();

    {
        const math::Vec4 proj_2 = _proj.row(2);
        const float a = proj_2[2];
        const float b = proj_2[3];
        if(a == 0.0f) {
            // far = inf
            // Using FLT_MAX doesn't give enough precision for anything, so we just use a big number
            camera_data.z_near = b;
            camera_data.z_far = float(1 << 18);
        } else {
            camera_data.z_near = b / (a + 1.0f);
            camera_data.z_far = b / a;
        }
    }

    return camera_data;
}

}

