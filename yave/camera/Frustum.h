/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_CAMERA_FRUSTUM_H
#define YAVE_CAMERA_FRUSTUM_H

#include <yave/meshes/AABB.h>

#include <array>

namespace yave {

enum class Intersection {
    Inside,
    Intersects,
    Outside
};

class Frustum {
    public:
        struct Plane {
            math::Vec3 normal;
            float offset = 0.0f;
        };

        enum Planes : usize {
            Near,
            Top,
            Bottom,
            Right,
            Left,
        };

        static Frustum from_view_proj(const math::Matrix4<>& view, const math::Matrix4<>& proj);
        static Frustum from_ortho_view_proj(const math::Matrix4<>& view, const math::Matrix4<>& proj);


        Frustum() = default;

        const math::Vec3& forward() const;
        const math::Vec3& position() const;

        const std::array<Plane, 5>& planes() const;

        bool is_inside(const math::Vec3& pos, float radius) const;

        Intersection intersection(const AABB& aabb) const;
        Intersection intersection(const AABB& aabb, float far_dist) const;
    private:
        std::array<Plane, 5> _planes;
        math::Vec3 _pos;

};

}

#endif // YAVE_CAMERA_FRUSTUM_H

