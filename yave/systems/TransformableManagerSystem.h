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
#ifndef YAVE_SYSTEMS_TRANSFORMABLEMANAGERSYSTEM_H
#define YAVE_SYSTEMS_TRANSFORMABLEMANAGERSYSTEM_H

#include <yave/ecs/System.h>
#include <yave/ecs/SparseComponentSet.h>

#include <yave/meshes/AABB.h>

#include <y/concurrent/Signal.h>

namespace yave {

class TransformableManagerSystem : public ecs::System {
    struct OctreeNode2 : NonCopyable {
        math::Vec3 center;
        float extent = 16.0f;

        core::Vector<u32> transforms;
        u32 children_index = u32(-1);

        AABB aabb() const;
    };

    struct TransformData {
        u32 parent_index = u32(-1);
        AABB global_aabb;

    };

    class Octree {
        public:
            static constexpr usize split_threshold = 32;
            static constexpr float min_node_extent = 1.0f;
            static constexpr float margin_factor = 1.25f;

            Octree();

            void insert(const TransformableComponent& tr);
            void remove(const TransformableComponent& tr);

            const OctreeNode2& parent_node(const TransformableComponent& tr) const;

        private:
            void insert_iterative(u32 node_index, u32 data_index);
            void split(u32 node_index);
            void recreate_root(const math::Vec3& toward);


            core::Vector<OctreeNode2> _nodes;
            core::Vector<TransformData> _datas;
    };

    public:
        struct TransformIndex {
            u32 index               : 31;
            u32 reset_transform     :  1;
        };

        static_assert(sizeof(TransformIndex) == sizeof(u32));

        TransformableManagerSystem();

        void destroy() override;
        void setup() override;
        void tick() override;


        u32 transform_count() const;

        AABB parent_node_aabb(const TransformableComponent& tr) const;

        const auto& moved() const {
            return _moved;
        }

        const auto& stopped() const {
            return _stopped;
        }

    private:
        void run_tick(bool only_recent);

        void free_index(const TransformableComponent& tr);
        TransformIndex alloc_index(const TransformableComponent& tr);

        void insert_into_octree(const TransformableComponent& tr);

        core::Vector<u32> _free;
        u32 _max_index = 0;

        ecs::SparseComponentSet<TransformIndex> _moved;
        ecs::SparseComponentSet<TransformIndex> _stopped;

        concurrent::Subscription _transform_destroyed;

        Octree _octree;


};

}

#endif // YAVE_SYSTEMS_TRANSFORMABLEMANAGERSYSTEM_H

