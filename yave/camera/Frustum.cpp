/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

namespace yave {

Frustum::Frustum(const std::array<math::Vec3, 4>& normals, const math::Vec3& pos, const math::Vec3& forward) : _pos(pos) {
    _normals[0] = forward;
    for(usize i = 0; i != normals.size(); ++i) {
        _normals[i + 1] = normals[i];
    }
}

bool Frustum::is_inside(const math::Vec3& pos, float radius) const {
    const math::Vec3 v = (pos - _pos);
    return v.dot(_normals[0]) + radius > 0.0f
        && v.dot(_normals[1]) + radius > 0.0f
        && v.dot(_normals[2]) + radius > 0.0f
        && v.dot(_normals[3]) + radius > 0.0f
        && v.dot(_normals[4]) + radius > 0.0f;
}

// https://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-testing-boxes-ii/
Intersection Frustum::intersection(const AABB &aabb) const {
    Intersection inter = Intersection::Inside;

    const math::Vec3 bbox_min = aabb.min() - _pos;
    const math::Vec3 bbox_max = aabb.max() - _pos;

    for(const math::Vec3& normal : _normals) {
        math::Vec3 p = bbox_min;
        math::Vec3 n = bbox_max;
        for(usize c = 0; c != 3; ++c) {
            if(normal[c] > 0.0f) {
                p[c] = bbox_max[c];
                n[c] = bbox_min[c];
            }
        }

        if(normal.dot(p) < 0.0f) {
            return Intersection::Outside;
        }
        if(normal.dot(n) < 0.0f) {
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

        const math::Vec3& normal = _normals[0];

        math::Vec3 p = bbox_min;
        math::Vec3 n = bbox_max;
        for(usize c = 0; c != 3; ++c) {
            if(normal[c] < 0.0f) {
                p[c] = bbox_max[c];
                n[c] = bbox_min[c];
            }
        }

        if(normal.dot(p) > -far_dist) {
            return Intersection::Outside;
        }
        if(normal.dot(n) > -far_dist) {
            return Intersection::Intersects;
        }
    }


    return inter;
}

}

