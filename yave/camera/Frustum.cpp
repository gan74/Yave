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

#include "Frustum.h"
#include "Camera.h"

namespace yave {

Frustum Frustum::from_view_proj(const math::Matrix4<>& view, const math::Matrix4<>& proj) {
    y_debug_assert(!Camera::is_proj_orthographic(proj));

    const float fov = Camera::fov_from_proj(proj);
    const float ratio = Camera::ratio_from_proj(proj);
    const math::Vec3 forward = Camera::forward_from_view(view);
    const math::Vec3 up = Camera::up_from_view(view);
    const math::Vec3 right = Camera::right_from_view(view);
    const math::Vec3 pos = Camera::position_from_view(view);

    y_debug_assert(fov > 0.0f);

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

    Frustum frustum;
    {
        frustum._pos = pos;
        frustum._planes[0] = {forward, 0.0f};
        for(usize i = 0; i != normals.size(); ++i) {
            frustum._planes[i + 1] = {normals[i], 0.0f};
        }
        y_debug_assert(frustum.is_inside(pos + forward, 0.0f));
    }
    return frustum;
}


Frustum Frustum::from_ortho_view_proj(const math::Matrix4<>& view, const math::Matrix4<>& proj) {
    y_debug_assert(Camera::is_proj_orthographic(proj));

    const math::Vec3 forward = Camera::forward_from_view(view);
    const math::Vec3 up = Camera::up_from_view(view);
    const math::Vec3 right = Camera::right_from_view(view);
    const math::Vec3 pos = Camera::position_from_view(view);

    const math::Matrix3<> viewproj = (proj * view).to<3, 3>();
    const math::Matrix3<> inv_viewproj = viewproj.inverse();

    math::Vec3 px = (inv_viewproj * math::Vec3( 1.0f, 0.0f, 0.0f)); // right
    math::Vec3 nx = (inv_viewproj * math::Vec3(-1.0f, 0.0f, 0.0f));
    math::Vec3 py = (inv_viewproj * math::Vec3(0.0f,  1.0f, 0.0f)); // up
    math::Vec3 ny = (inv_viewproj * math::Vec3(0.0f, -1.0f, 0.0f));
    math::Vec3 pz = (inv_viewproj * math::Vec3(0.0f, 0.0f,  1.0f)); // forward
    math::Vec3 nz = (inv_viewproj * math::Vec3(0.0f, 0.0f, -1.0f));

    if(px.dot(right) < 0.0f) {
        std::swap(px, nx);
    }
    if(py.dot(up) < 0.0f) {
        std::swap(py, ny);
    }
    if(pz.dot(forward) < 0.0f) {
        std::swap(pz, nz);
    }

    Frustum frustum;
    {
        auto make_plane = [](const math::Vec3 v) -> Plane {
            const float len = v.length();
            return {v / len, -len};
        };

        frustum._pos = pos;
        frustum._planes[Frustum::Near] = make_plane(pz);
        frustum._planes[Frustum::Top] = make_plane(py);
        frustum._planes[Frustum::Bottom] = make_plane(ny);
        frustum._planes[Frustum::Right] = make_plane(px);
        frustum._planes[Frustum::Left] = make_plane(nx);

        y_debug_assert(frustum.is_inside(pos, 0.0f));
        y_debug_assert(!frustum.is_inside(pos - forward * nz.length() * 1.1f, 0.0f));
    }
    return frustum;
}


const math::Vec3& Frustum::forward() const {
    return _planes[0].normal;
}

const math::Vec3& Frustum::position() const {
    return _pos;
}

const std::array<Frustum::Plane, 5>& Frustum::planes() const {
    return _planes;
}

bool Frustum::is_inside(const math::Vec3& pos, float radius) const {
    const math::Vec3 v = (pos - _pos);
    return v.dot(_planes[0].normal) + radius > _planes[0].offset
        && v.dot(_planes[1].normal) + radius > _planes[1].offset
        && v.dot(_planes[2].normal) + radius > _planes[2].offset
        && v.dot(_planes[3].normal) + radius > _planes[3].offset
        && v.dot(_planes[4].normal) + radius > _planes[4].offset;
}

// https://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-testing-boxes-ii/
Intersection Frustum::intersection(const AABB &aabb) const {
    Intersection inter = Intersection::Inside;

    const math::Vec3 bbox_min = aabb.min() - _pos;
    const math::Vec3 bbox_max = aabb.max() - _pos;


    for(const Plane plane : _planes) {
        math::Vec3 p = bbox_min;
        math::Vec3 n = bbox_max;
        for(usize c = 0; c != 3; ++c) {
            if(plane.normal[c] > 0.0f) {
                p[c] = bbox_max[c];
                n[c] = bbox_min[c];
            }
        }

        if(plane.normal.dot(p) < plane.offset) {
            return Intersection::Outside;
        }
        if(plane.normal.dot(n) < plane.offset) {
            inter = Intersection::Intersects;
        }
    }

    return inter;
}

Intersection Frustum::intersection(const AABB &aabb, float far_dist) const {
    const Intersection inter = intersection(aabb);

    // Not exactly efficient
    if(inter != Intersection::Outside && far_dist > 0.0f) {
        const math::Vec3 bbox_min = aabb.min() - _pos;
        const math::Vec3 bbox_max = aabb.max() - _pos;

        const math::Vec3 normal = _planes[0].normal;

        math::Vec3 p = bbox_min;
        math::Vec3 n = bbox_max;
        for(usize c = 0; c != 3; ++c) {
            if(normal[c] < 0.0f) {
                p[c] = bbox_max[c];
                n[c] = bbox_min[c];
            }
        }

        if(normal.dot(p) > far_dist) {
            return Intersection::Outside;
        }
        if(normal.dot(n) > far_dist) {
            return Intersection::Intersects;
        }
    }


    return inter;
}


}

