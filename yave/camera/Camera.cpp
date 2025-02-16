/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

bool Camera::is_proj_orthographic(const math::Matrix4<>& proj) {
    return proj[3][3] == 1.0f;
}

float Camera::ratio_from_proj(const math::Matrix4<>& proj) {
    const float f = proj[1][1];
    return std::abs(1.0f / (proj[0][0] / f));
}

float Camera::fov_from_proj(const math::Matrix4<>& proj) {
    const float f = proj[1][1];
    return 2.0f * std::atan(1.0f / f);
}

math::Vec3 Camera::forward_from_view(const math::Matrix4<>& view) {
    return -view.row(2).to<3>().normalized();
}

math::Vec3 Camera::right_from_view(const math::Matrix4<>& view) {
    return -view.row(0).to<3>().normalized();
}

math::Vec3 Camera::up_from_view(const math::Matrix4<>& view) {
    return -view.row(1).to<3>().normalized();
}

math::Vec3 Camera::position_from_view(const math::Matrix4<>& view) {
    math::Vec3 pos;
    for(usize i = 0; i != 3; ++i) {
        const auto v = view.row(i);
        pos -= v.to<3>() * v.w();
    }
    return pos;
}



Camera::Camera() : Camera(math::look_at({2.0f, 0.0f, 0.0f}, math::Vec3{}, math::Vec3{0.0f, 0.0f, 1.0f}), math::perspective(math::to_rad(45.0f), 16.0f / 9.0f, 0.1f)) {
}

Camera::Camera(const math::Matrix4<>& view, const math::Matrix4<>& proj) : _view(view), _proj(proj) {
    update_view_proj();
}

void Camera::set_view(const math::Matrix4<>& view) {
    _view = view;
    update_view_proj();
}

void Camera::set_proj(const math::Matrix4<>& proj) {
    _proj = proj;
    update_view_proj();
}

void Camera::update_view_proj() {
    _view_proj = _proj * _view;
}

void Camera::set_far(float far_dist) {
    _far = far_dist;
}


float Camera::aspect() const {
    return _proj[1][1] / _proj[0][0];
}

math::Vec2 Camera::jitter() const {
    return _proj[2].to<2>();
}


Camera Camera::jittered(math::Vec2 jitter_seq, const math::Vec2ui& size, float intensity) const {
    y_debug_assert(jitter_seq.saturated() == jitter_seq);
    y_debug_assert(!size.is_zero());

    const math::Vec2 jitter = ((jitter_seq  * 2.0f - 1.0f) / math::Vec2(size)) * intensity;

    math::Matrix4<> jittered_proj = _proj;
    jittered_proj[2].to<2>() = jitter;

    return Camera(_view, jittered_proj);
}

Camera Camera::unjittered() const {
    return Camera(_view, unjittered_proj());
}

const math::Matrix4<>& Camera::view_matrix() const {
    return _view;
}

const math::Matrix4<>& Camera::proj_matrix() const {
    return _proj;
}

const math::Matrix4<>& Camera::view_proj_matrix() const {
    return _view_proj;
}

math::Matrix4<> Camera::inverse_matrix() const {
    return (view_proj_matrix()).inverse();
}

math::Matrix4<> Camera::unjittered_proj() const {
    math::Matrix4<> proj = _proj;
    proj[2].to<2>() = {};
    return proj;
}

math::Matrix4<> Camera::unjittered_view_proj() const {
    return unjittered_proj() * _view;
}

float Camera::aspect_ratio() const {
    return ratio_from_proj(_proj);
}

float Camera::field_of_view() const {
    return fov_from_proj(_proj);
}

bool Camera::is_orthographic() const {
    return is_proj_orthographic(_proj);
}

float Camera::far_plane_dist() const {
    return _far;
}

math::Vec3 Camera::position() const {
    return position_from_view(_view);
}

math::Vec3 Camera::forward() const {
    return forward_from_view(_view);
}

math::Vec3 Camera::right() const {
    return right_from_view(_view);
}

math::Vec3 Camera::up() const {
    return up_from_view(_view);
}

Frustum Camera::frustum() const {
    return is_orthographic()
        ? Frustum::from_ortho_view_proj(view_matrix(), proj_matrix())
        : Frustum::from_view_proj(view_matrix(), proj_matrix());
}

Camera::operator shader::Camera() const {
    shader::Camera camera_data = {};

    camera_data.cur.view_proj = view_proj_matrix();
    camera_data.cur.inv_view_proj = inverse_matrix();

    camera_data.prev = camera_data.cur;

    camera_data.jitter = jitter();
    camera_data.prev_jitter = camera_data.jitter;

    camera_data.proj = proj_matrix();
    camera_data.inv_proj = camera_data.proj.inverse();

    camera_data.view = view_matrix();
    camera_data.inv_view = camera_data.view.inverse();

    camera_data.position = position();
    camera_data.forward = forward();
    camera_data.up = up();

    camera_data.aspect = aspect();

    return camera_data;
}

}

