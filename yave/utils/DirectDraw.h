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
#ifndef YAVE_UTILS_DIRECTDRAW_H
#define YAVE_UTILS_DIRECTDRAW_H

#include <yave/yave.h>

#include <yave/meshes/AABB.h>

#include <y/core/Vector.h>

#include <memory>

namespace yave {

class DirectDrawPrimitive {
    public:
        void add_circle(const math::Vec3& position, math::Vec3 x, math::Vec3 y, float radius = 1.0f, usize divs = 64);
        void add_cone(const math::Vec3& position, math::Vec3 x, math::Vec3 y, float len, float angle, usize divs = 8, usize circle_subdivs = 8);
        void add_box(const AABB& aabb, const math::Transform<>& transform = {});

        core::Span<math::Vec3> points() const;

    private:
        friend class DirectDraw;

        math::Vec3 _color = math::Vec3(0, 0, 1);
        core::Vector<math::Vec3> _points;
};

class DirectDraw : NonCopyable {

    public:
        DirectDrawPrimitive* add_primitive(const math::Vec3& color = math::Vec3(0, 0, 1));

        core::Span<std::unique_ptr<DirectDrawPrimitive>> primtitives() const;

    private:
        core::Vector<std::unique_ptr<DirectDrawPrimitive>> _primitives;


};

}


#endif // YAVE_UTILS_DIRECTDRAW_H

