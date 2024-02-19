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

#include <yave/systems/AABBUpdateSystem.h>

#include <yave/meshes/AABB.h>

#include <y/reflect/reflect.h>

namespace yave {

class TransformableComponent final {
    public:
        TransformableComponent(const math::Transform<>& transform = {});
        ~TransformableComponent();

        TransformableComponent(TransformableComponent&& other);
        TransformableComponent& operator=(TransformableComponent&& other);

        TransformableComponent(const TransformableComponent& other);
        TransformableComponent& operator=(const TransformableComponent& other);

        void set_transform(const math::Transform<>& tr);
        void set_position(const math::Vec3& pos);

        const math::Transform<>& transform() const;

        const math::Vec3& forward() const;
        const math::Vec3& right() const;
        const math::Vec3& up() const;

        const math::Vec3& position() const;

        math::Vec3 to_global(const math::Vec3& pos) const;
        AABB to_global(const AABB& aabb) const;

        void set_aabb(const AABB& aabb);
        const AABB& local_aabb() const;
        AABB global_aabb() const;

        const OctreeNode* octree_node() const;
        u32 transform_index() const;

        void inspect(ecs::ComponentInspector* inspector);

        y_reflect(TransformableComponent, _transform)

    private:
        void swap(TransformableComponent& other);

        math::Transform<> _transform;
        AABB _aabb;

        friend class Octree;
        friend class OctreeSystem;
        mutable OctreeNode* _node = nullptr;


    private:
        friend class TransformableManagerSystem;

        mutable u32 _transform_index = u32(-1);
};

}

#endif // YAVE_COMPONENTS_TRANSFORMABLECOMPONENT_H

