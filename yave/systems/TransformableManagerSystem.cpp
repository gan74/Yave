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

#include "TransformableManagerSystem.h"

#include <yave/camera/Frustum.h>
#include <yave/components/TransformableComponent.h>

#include <yave/ecs/EntityWorld.h>


namespace yave {

static u32 compute_child_index(const math::Vec3& center, const math::Vec3& pos) {
    u32 index = 0;
    for(u32 i = 0; i != 3; ++i) {
        if(pos[i] > center[i]) {
            index += (1u << i);
        }
    }
    return index;
}



AABB TransformableManagerSystem::OctreeNode::aabb() const {
    return AABB::from_center_extent(center, math::Vec3(extent * Octree::margin_factor));
}



TransformableManagerSystem::Octree::Octree() {
    _nodes.emplace_back();
}

core::Vector<ecs::EntityId> TransformableManagerSystem::Octree::find_visible(const Frustum& frustum, float far_dist) const {
    y_profile();

    auto entities = core::Vector<ecs::EntityId>::with_capacity(_datas.size());
    visit_node(entities, 0u, frustum, far_dist);
    return entities;
}

const TransformableManagerSystem::OctreeNode& TransformableManagerSystem::Octree::parent_node(const TransformableComponent& tr) const {
    y_debug_assert(tr._transform_index != u32(-1));
    const TransformData& data = _datas[tr._transform_index];
    y_debug_assert(data.parent_index != u32(-1));
    return _nodes[data.parent_index];
}

void TransformableManagerSystem::Octree::insert_or_update(ecs::EntityId id, const TransformableComponent& tr) {
    _datas.set_min_size(tr._transform_index + 1);

    TransformData& data = _datas[tr._transform_index];

    y_debug_assert(!data.id.is_valid() || data.id == id);

    data.global_aabb = tr.global_aabb();
    data.id = id;

    if(data.parent_index != u32(-1)) {
        if(const OctreeNode& parent = _nodes[data.parent_index]; parent.aabb().contains(data.global_aabb)) {
           return;
        }
        remove(tr);
    }

    // Avoid invalidation during iteration if we add children
    const usize max_realloc = split_threshold * split_threshold + 1;
    _nodes.set_min_capacity(_nodes.size() + max_realloc);

    while(!_nodes[0].aabb().contains(data.global_aabb)) {
        recreate_root(data.global_aabb.center());

        _nodes.set_min_capacity(_nodes.size() + max_realloc);
    }

    insert_iterative(0, tr._transform_index);
    y_debug_assert(data.parent_index != u32(-1));
}

void TransformableManagerSystem::Octree::remove(const TransformableComponent& tr) {
    y_profile();

    const u32 index = tr._transform_index;

    TransformData& data = _datas[index];
    y_debug_assert(data.parent_index != u32(-1));

    OctreeNode& node = _nodes[data.parent_index];
    const auto it = std::find(node.transforms.begin(), node.transforms.end(), index);
    y_debug_assert(it != node.transforms.end());
    node.transforms.erase_unordered(it);

    data.parent_index = u32(-1);
    data.id = {};
}


void TransformableManagerSystem::Octree::insert_iterative(u32 node_index, u32 data_index) {
    y_profile();

    TransformData& data = _datas[data_index];
    const AABB aabb = data.global_aabb;

    for(;;) {
        OctreeNode& node = _nodes[node_index];

        y_debug_assert(data.parent_index == u32(-1));
        y_debug_assert(node.aabb().contains(aabb));

        const bool should_split = node.children_index == u32(-1) && node.transforms.size() >= split_threshold;
        if(should_split && node.extent > min_node_extent) {
            split(node_index);
            reinsert(node_index);
        }

        if(node.children_index != u32(-1)) {
            const u32 child_index = node.children_index + compute_child_index(node.center, aabb.center());
            if(_nodes[child_index].aabb().contains(aabb)) {
                node_index = child_index;
                continue;
            }
        }

        node.transforms << data_index;
        data.parent_index = u32(node_index);
        break;
    }
}

void TransformableManagerSystem::Octree::split(u32 node_index) {
    y_profile();

    OctreeNode& node = _nodes[node_index];

    y_debug_assert(node.children_index == u32(-1));

    node.children_index = u32(_nodes.size());
    _nodes.set_min_size(_nodes.size() + 8);

    y_debug_assert(&node == &_nodes[node_index]);

    const float child_extent = node.extent * 0.5f;
    for(usize i = 0; i != 8; ++i) {
        const math::Vec3 offset = math::Vec3(
            (i & 0x01 ? child_extent : -child_extent),
            (i & 0x02 ? child_extent : -child_extent),
            (i & 0x04 ? child_extent : -child_extent)
        ) * 0.5f;

        OctreeNode& child = _nodes[node.children_index + i];
        {
            child.center = node.center + offset;
            child.extent = child_extent;
        }
        y_debug_assert(node.aabb().contains(child.aabb()));
    }
}

void TransformableManagerSystem::Octree::reinsert(u32 node_index) {
    OctreeNode& node = _nodes[node_index];

    for(usize i = 0; i != node.transforms.size(); ++i) {
        const u32 index = node.transforms[i];
        TransformData& data = _datas[index];
        const AABB aabb = data.global_aabb;
        const u32 child_index = node.children_index + compute_child_index(node.center, aabb.center());
        if(_nodes[child_index].aabb().contains(aabb)) {
            data.parent_index = u32(-1);
            insert_iterative(child_index, index);
            node.transforms.erase_unordered(node.transforms.begin() + i);
            --i;
        }
    }
}

void TransformableManagerSystem::Octree::recreate_root(const math::Vec3& toward) {
    y_profile();

    OctreeNode& root = _nodes[0];

    [[maybe_unused]]
    const AABB orig_aabb = AABB::from_center_extent(root.center, math::Vec3(root.extent));
    const u32 children_index = root.children_index;

    const usize index = compute_child_index(root.center, toward);
    const float child_extent = root.extent;
    const math::Vec3 parent_offset = math::Vec3(
        (index & 0x01 ? child_extent : -child_extent),
        (index & 0x02 ? child_extent : -child_extent),
        (index & 0x04 ? child_extent : -child_extent)
    ) * 0.5f;

    {
        root.center += parent_offset;
        root.extent *= 2.0f;
        root.children_index = u32(-1);
    }

    split(0);

    y_debug_assert(root.children_index != u32(-1));

    const u32 child_index = root.children_index + u32(7 - index);
    OctreeNode& child = _nodes[child_index];

    y_debug_assert(child.aabb().contains(orig_aabb));

    {
        child.transforms.swap(root.transforms);
        child.children_index = children_index;

        for(const u32 data_index : child.transforms) {
            _datas[data_index].parent_index = u32(child_index);
        }
    }

    y_debug_assert(std::all_of(child.transforms.begin(), child.transforms.end(), [&](u32 index) {
        return child.aabb().contains(_datas[index].global_aabb);
    }));
}


void TransformableManagerSystem::Octree::visit_node(core::Vector<ecs::EntityId>& entities, u32 node_index, const Frustum& frustum, float far_dist) const {
    const OctreeNode& node = _nodes[node_index];

    switch(frustum.intersection(node.aabb(), far_dist)) {
        case Intersection::Outside:
        break;

        case Intersection::Inside:
            push_all_entities(entities, node_index);
        break;

        case Intersection::Intersects:
            for(const u32 index : node.transforms) {
                const TransformData& data = _datas[index];
                switch(frustum.intersection(data.global_aabb, far_dist)) {
                    case Intersection::Inside:
                    case Intersection::Intersects:
                        entities << data.id;
                    break;

                    default:
                    break;
                }
            }

            if(node.children_index != u32(-1)) {
                for(u32 i = 0; i != 8; ++i) {
                    visit_node(entities, node.children_index + i, frustum, far_dist);
                }
            }
        break;
    }
}

void TransformableManagerSystem::Octree::push_all_entities(core::Vector<ecs::EntityId>& entities, u32 node_index) const {
    const OctreeNode& node = _nodes[node_index];

    for(const u32 index : node.transforms) {
        const TransformData& data = _datas[index];
        entities << data.id;
    }

    if(node.children_index != u32(-1)) {
        for(u32 i = 0; i != 8; ++i) {
            push_all_entities(entities, node.children_index + i);
        }
    }
}






TransformableManagerSystem::TransformableManagerSystem() : ecs::System("TransformableManagerSystem") {
}

void TransformableManagerSystem::destroy() {
    _octree = {};
    _transform_destroyed.disconnect();
    auto query = world().query<TransformableComponent>();
    for(const auto& [tr] : query.components()) {
        free_index(tr);
    }
}

void TransformableManagerSystem::setup() {
    _transform_destroyed = world().on_destroyed<TransformableComponent>().subscribe([this](ecs::EntityId, TransformableComponent& tr) {
        free_index(tr);
    });

    run_tick(false);
}

void TransformableManagerSystem::tick() {
    run_tick(true);
}

void TransformableManagerSystem::run_tick(bool only_recent) {
    y_profile();

    auto moved_query = only_recent ? world().query<ecs::Changed<TransformableComponent>>() : world().query<TransformableComponent>();

    _stopped.swap(_moved);
    _moved.make_empty();

    for(const auto& [id, comp] : moved_query.id_components()) {
        const auto& [tr] = comp;
        _moved.insert(id, alloc_index(tr));
        _stopped.erase(id);

        _octree.insert_or_update(id, tr);
    }
}


void TransformableManagerSystem::free_index(const TransformableComponent& tr) {
    if(tr._transform_index != u32(-1)) {
        _free.push_back(tr._transform_index);
        tr._transform_index = u32(-1);
    }
}

TransformableManagerSystem::TransformIndex TransformableManagerSystem::alloc_index(const TransformableComponent& tr) {
    if(tr._transform_index == u32(-1)) {
        if(!_free.is_empty()) {
            tr._transform_index = _free.pop();
        } else {
            tr._transform_index = _max_index++;
        }

        return TransformIndex {
            tr._transform_index, true,
        };
    }

    return TransformIndex {
        tr._transform_index, false,
    };
}

u32 TransformableManagerSystem::transform_count() const {
    return _max_index;
}

AABB TransformableManagerSystem::parent_node_aabb(const TransformableComponent& tr) const {
    return _octree.parent_node(tr).aabb();
}

core::Vector<ecs::EntityId> TransformableManagerSystem::find_visible(const Frustum& frustum, float far_dist) const {
    return _octree.find_visible(frustum, far_dist);
}

}

