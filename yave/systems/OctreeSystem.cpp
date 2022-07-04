/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#include "AssetLoaderSystem.h"

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

static AABB find_aabb(const ecs::EntityWorld& world, ecs::EntityId id, const math::Vec3& pos) {
    const auto aabb = entity_aabb(world, id);
    if(aabb) {
        return aabb.unwrap();
    }

    const float radius = entity_radius(world, id).unwrap_or(1.0f);
    return AABB::from_center_extent(pos, math::Vec3(radius * 2.0f));
}



OctreeSystem::OctreeSystem() : ecs::System("OctreeSystem") {
}

void OctreeSystem::destroy(ecs::EntityWorld& world) {
    for(auto&& [tr] : world.query<ecs::Mutate<TransformableComponent>>().components()) {
        tr._id.invalidate();
        tr._node = nullptr;
    }
}

void OctreeSystem::setup(ecs::EntityWorld& world) {
    run_tick(world, false);
}

void OctreeSystem::tick(ecs::EntityWorld& world) {
    run_tick(world, true);
}

void OctreeSystem::run_tick(ecs::EntityWorld& world, bool only_recent) {
    y_profile();

    Y_TODO(notify system instead?)
    if(const AssetLoaderSystem* asset_loader = world.find_system<AssetLoaderSystem>()) {
        for(auto&& [tr] : world.query<ecs::Mutate<TransformableComponent>>(asset_loader->recently_loaded()).components()) {
            tr.dirty_node();
        }
    }

    for(auto&& id_comp : world.query<ecs::Mutate<TransformableComponent>>(transformable_ids(world, only_recent)).id_components()) {
        const auto id = id_comp.id();
        auto& tr = id_comp.component<TransformableComponent>();

        const AABB bbox = find_aabb(world, id, tr.position());

        tr._id = id;
        tr.set_node(_tree.insert(id, bbox));
    }


    {
        auto& transformables = world.component_set<TransformableComponent>();
        for(auto& [node, id] : _tree._data._dirty) {
            {
                auto& entities = node->_entities;
                const auto it = std::find(entities.begin(), entities.end(), id);
                if(it != entities.end()) {
                    entities.erase_unordered(it);
                }
            }

            if(TransformableComponent* tr = transformables.try_get(id)) {
                const AABB bbox = find_aabb(world, id, tr->position());
                y_debug_assert(tr->_id == id);
                tr->set_node(_tree.insert(id, bbox));
            }
        }

        _tree._data._dirty.clear();
    }
}

const OctreeNode& OctreeSystem::root() const {
    return _tree.root();
}

const Octree& OctreeSystem::octree() const {
    return _tree;
}

}

