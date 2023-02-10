/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "OctreeNode.h"

namespace yave {

OctreeNode::OctreeNode(const math::Vec3& center, float half_extent, OctreeData* data) :
        _center(center),
        _half_extent(half_extent),
        _data(data) {
}

std::unique_ptr<OctreeNode> OctreeNode::create_parent_from_child(std::unique_ptr<OctreeNode> child, const math::Vec3& toward) {
    const usize index = child->children_index(toward);
    const float child_half_extent = child->_half_extent;
    const math::Vec3 parent_offset = {
        (index & 0x01 ? child_half_extent : -child_half_extent),
        (index & 0x02 ? child_half_extent : -child_half_extent),
        (index & 0x04 ? child_half_extent : -child_half_extent)
    };

    auto parent = std::make_unique<OctreeNode>(child->_center + parent_offset, child_half_extent * 2.0f, child->_data);
    parent->build_children();

    const usize self_index = 7 - index;
    y_debug_assert(parent->children_index(child->_center) == self_index);

    parent->_children[self_index] = std::move(child);
    return parent;
}


OctreeNode* OctreeNode::insert(ecs::EntityId id, const AABB& bbox) {
    y_debug_assert(id.is_valid());
    y_debug_assert(contains(bbox));

    y_debug_assert(std::find(_entities.begin(), _entities.end(), id) == _entities.end());

    const bool split_small = split_small_object && bbox.half_extent().length() < _half_extent / min_object_size_ratio;
    const bool should_insert_into_children = _entities.size() >= max_entities_per_node || split_small;
    if(!has_children() && should_insert_into_children && _half_extent * 2.0f > min_node_extent) {
        build_children();
    }

    if(has_children()) {
        const usize index = children_index(bbox.center());
        if(_children[index]->contains(bbox)) {
            return _children[index]->insert(id, bbox);
        }
    }

    _entities << id;

    return this;
}

AABB OctreeNode::aabb() const {
    return AABB::from_center_extent(_center, math::Vec3(_half_extent * full_extent_multiplier));
}

AABB OctreeNode::strict_aabb() const {
    return AABB::from_center_extent(_center, math::Vec3(_half_extent * 2.0f));
}

bool OctreeNode::contains(const AABB& bbox) const {
    return aabb().contains(bbox);
}


bool OctreeNode::contains(const math::Vec3& pos, float radius) const {
    const math::Vec3 to_center = (pos - _center).abs();

    const float max_dist = _half_extent - radius;
    for(float t : to_center) {
        if(t > max_dist) {
            return false;
        }
    }
    return true;
}

bool OctreeNode::has_children() const {
    return _children[0] != nullptr;
}

bool OctreeNode::is_empty() const {
    return !has_children() && _entities.is_empty();
}

core::Span<std::unique_ptr<OctreeNode>> OctreeNode::children() const {
    return has_children()
        ? core::Span<std::unique_ptr<OctreeNode>>(_children.data(), _children.size())
        : core::Span<std::unique_ptr<OctreeNode>>();
}

core::Span<ecs::EntityId> OctreeNode::entities() const {
    return _entities;
}

void OctreeNode::remove(ecs::EntityId id) {
    y_debug_assert(id.is_valid());
    const auto it = std::find(_entities.begin(), _entities.end(), id);
    y_debug_assert(it != _entities.end());
    _entities.erase_unordered(it);
}

void OctreeNode::build_children() {
    y_debug_assert(!has_children());

    const float child_extent = _half_extent * 0.5f;

    for(usize i = 0; i != 8; ++i) {
        const math::Vec3 offset = {
            (i & 0x01 ? child_extent : -child_extent),
            (i & 0x02 ? child_extent : -child_extent),
            (i & 0x04 ? child_extent : -child_extent)
        };

        _children[i] = std::make_unique<OctreeNode>(_center + offset, child_extent, _data);
        y_debug_assert(contains(_children[i]->aabb()));
    }
}

usize OctreeNode::children_index(const math::Vec3& pos) {
    usize index = 0;
    for(usize i = 0; i != 3; ++i) {
        if(pos[i] > _center[i]) {
            index += (1_uu << i);
        }
    }
    return index;
}

usize OctreeNode::entity_count() const {
    usize count = _entities.size();
    for(const auto& child : children()) {
        count += child->entity_count();
    }
    return count;
}

}

