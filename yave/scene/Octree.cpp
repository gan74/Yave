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

#include "Octree.h"

#include <yave/camera/Frustum.h>

#include <y/utils/format.h>

namespace yave {

static void push_all_entities(core::Vector<ecs::EntityId>& entities, const OctreeNode& node) {
    for(const auto& p : node.entities()) {
        entities << p.first;
    }

    for(const auto& child : node.children()) {
        push_all_entities(entities, *child);
    }
}

static void visit_node(core::Vector<ecs::EntityId>& query, const Frustum& frustum, float far_dist, const OctreeNode& node) {
    switch(frustum.intersection(node.aabb(), far_dist)) {
        case Intersection::Outside:
        break;

        case Intersection::Inside:
            push_all_entities(query, node);
        break;

        case Intersection::Intersects:
            for(const auto& p : node.entities()) {
                switch(frustum.intersection(p.second, far_dist)) {
                    case Intersection::Inside:
                    case Intersection::Intersects:
                        query << p.first;
                    break;

                    default:
                    break;
                }

            }
            for(const auto& child : node.children()) {
                visit_node(query, frustum, far_dist, *child);
            }
        break;
    }
}

Octree::Octree() : _root(std::make_unique<OctreeNode>(math::Vec3(), 8.0f, &_data)) {
}

OctreeNode* Octree::insert(ecs::EntityId id, const AABB& bbox) {
    y_debug_assert(id.is_valid());

    while(!_root->contains(bbox)) {
        const math::Vec3 pos = bbox.center();
        _root = OctreeNode::create_parent_from_child(std::move(_root), pos);
    }
    return _root->insert(id, bbox);
}

const OctreeNode& Octree::root() const {
    return *_root;
}

core::Vector<ecs::EntityId> Octree::all_entities() const {
    y_profile();

    auto entities = core::Vector<ecs::EntityId>::with_capacity(1024 * 8);
    push_all_entities(entities, *_root);
    return entities;
}

core::Vector<ecs::EntityId> Octree::find_entities(const Frustum& frustum, float far_dist) const {
    y_profile();

    auto query = core::Vector<ecs::EntityId>::with_capacity(1024);
    visit_node(query, frustum, far_dist, *_root);
    return query;
}

void Octree::audit() const {
#ifdef Y_DEBUG
    y_profile();
    core::Vector<ecs::EntityId> all = all_entities();
    y_profile_dyn_zone(fmt_c_str("auditing {} entities", all.size()));
    std::sort(all.begin(), all.end());
    y_debug_assert(_root->all_entity_count() == all.size());
    y_debug_assert(std::unique(all.begin(), all.end()) == all.end());
#endif
}

}

