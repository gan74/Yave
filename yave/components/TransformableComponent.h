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
#ifndef YAVE_COMPONENTS_TRANSFORMABLECOMPONENT_H
#define YAVE_COMPONENTS_TRANSFORMABLECOMPONENT_H

#include <yave/meshes/AABB.h>

#include <y/reflect/reflect.h>

namespace yave {

class TransformableComponent final {
    public:
        TransformableComponent(const math::Transform<>& transform = {});

        void set_transform(const math::Transform<>& tr);
        void set_position(const math::Vec3& pos);

        const math::Transform<>& transform() const;

        const math::Vec3& forward() const;
        const math::Vec3& right() const;
        const math::Vec3& up() const;

        const math::Vec3& position() const;

        math::Vec3 to_global(const math::Vec3& pos) const;
        AABB to_global(const AABB& aabb) const;

        void inspect(ecs::ComponentInspector* inspector);

        y_reflect(TransformableComponent, _transform)

    private:
        math::Transform<> _transform;
};

}

#endif // YAVE_COMPONENTS_TRANSFORMABLECOMPONENT_H

