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
#ifndef YAVE_SCENE_OCTREENODE_H
#define YAVE_SCENE_OCTREENODE_H

#include "OctreeData.h"

#include <yave/meshes/AABB.h>

#include <array>
#include <memory>

namespace yave {


class OctreeNode {

    using Children = std::array<OctreeNode, 8>;

    static constexpr bool split_small_object = false;
    static constexpr float min_object_size_ratio = 16.0f;

    static constexpr usize max_entities_per_node = 32;
    static constexpr float min_node_extent = 1.0f;

    static constexpr float overlap_extent_multiplier = 1.25f;
    static constexpr float full_extent_multiplier = overlap_extent_multiplier * 2.0f;

    public:
        OctreeNode() = default;
        OctreeNode(const math::Vec3& center, float half_extent, OctreeData* data);

        std::pair<OctreeNode*, OctreeEntityId> insert(const AABB& bbox);

        AABB aabb() const;
        AABB strict_aabb() const;

        bool contains(const AABB& bbox) const;
        bool contains(const math::Vec3& pos, float radius) const;

        bool has_children() const;
        bool is_empty() const;

        core::Span<OctreeNode> children() const;

        Y_TODO(Make thread safe)
        void set_dirty(OctreeEntityId id);

    private:
        friend class Octree;

        void into_parent(const math::Vec3& toward);

    private:
        void swap(OctreeNode& other);

        void build_children();
        usize children_index(const math::Vec3& pos);

        math::Vec3 _center;
        float _half_extent = -1.0f;

        std::unique_ptr<Children> _children;

        core::Vector<OctreeEntityId> _entities;

        OctreeData* _data = nullptr;
};

}

#endif // YAVE_SCENE_OCTREENODE_H

