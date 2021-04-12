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

#include "OctreeSystem.h"

#include <yave/components/TransformableComponent.h>

#include <yave/ecs/EntityWorld.h>

#include <yave/utils/entities.h>

#include <y/core/Chrono.h>

namespace yave {

static core::Span<ecs::EntityId> transformable_ids(ecs::EntityWorld& world, bool recent) {
    return recent
        ? world.recently_added<TransformableComponent>()
        : world.component_ids<TransformableComponent>();
}


OctreeSystem::OctreeSystem() : ecs::System("OctreeSystem") {
}

void OctreeSystem::setup(ecs::EntityWorld& world) {
    run_tick(world, false);
}

void OctreeSystem::tick(ecs::EntityWorld& world) {
    run_tick(world, true);
}

void OctreeSystem::run_tick(ecs::EntityWorld& world, bool only_recent) {

    // TEMP TEMP TEMP TEMP
    {
        _tree.~Octree();
        new(&_tree) Octree();
        only_recent = false;
    }


    for(auto&& id_comp  : world.view<TransformableComponent>(transformable_ids(world, only_recent)).id_components()) {
        const auto id = id_comp.id();
        auto& tr = id_comp.component<TransformableComponent>();

        const float radius = entity_radius(world, id).unwrap_or(1.0f);
        const AABB bbox = AABB::from_center_extent(tr.position(), math::Vec3(radius * 2.0f));
        const auto [node, octree_id] = _tree.insert(bbox);

        tr._node_id = octree_id;
        tr._node = node;
    }
}

const OctreeNode& OctreeSystem::root() const {
    return _tree.root();
}

}

