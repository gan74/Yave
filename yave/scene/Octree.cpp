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

#include "Octree.h"

#include <yave/camera/Frustum.h>

namespace yave {

static void push_all_entities(core::Vector<ecs::EntityId>& entities, const OctreeNode& node) {
    for(const ecs::EntityId id : node.entities()) {
        entities << id;
    }
    for(const OctreeNode& child : node.children()) {
        push_all_entities(entities, child);
    }
}

static void visit_node(core::Vector<ecs::EntityId>& entities, const Frustum& frustum, const OctreeNode& node) {
    switch(frustum.intersection(node.aabb())) {
        case Intersection::Outside:
        break;

        case Intersection::Inside:
            push_all_entities(entities, node);
        break;

        case Intersection::Intersects:
            for(const ecs::EntityId id : node.entities()) {
                entities << id;
            }
            for(const OctreeNode& child : node.children()) {
                visit_node(entities, frustum, child);
            }
        break;
    }
}



Octree::Octree() : _root({}, 1024.0, &_data) {
}

OctreeNode* Octree::insert(ecs::EntityId id, const AABB& bbox) {
    y_profile();

    const math::Vec3 pos = bbox.center();
    while(!_root.contains(bbox)) {
        _root.into_parent(pos);
    }
    return _root.insert(id, bbox);
}

const OctreeNode& Octree::root() const {
    return _root;
}

core::Vector<ecs::EntityId> Octree::find_entities(const Frustum& frustum) const {
    y_profile();

    core::Vector<ecs::EntityId> entities;
    visit_node(entities, frustum, _root);
    return entities;
}

}

