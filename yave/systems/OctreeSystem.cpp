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

#include "OctreeSystem.h"
#include "AssetLoaderSystem.h"

#include <yave/components/TransformableComponent.h>
#include <yave/ecs/EntityWorld.h>

#include <y/core/Chrono.h>

namespace yave {

static core::Span<ecs::EntityId> transformable_ids(ecs::EntityWorld& world, bool recent) {
    return recent
        ? world.recently_mutated<TransformableComponent>()
        : world.component_ids<TransformableComponent>();
}



OctreeSystem::OctreeSystem() : ecs::System("OctreeSystem") {
}

void OctreeSystem::destroy() {
    auto query = world().query<ecs::Mutate<TransformableComponent>>();
    for(auto&& [tr] : query.components()) {
        tr._node = nullptr;
    }
}

void OctreeSystem::setup() {
    run_tick(false);
}

void OctreeSystem::tick() {
    run_tick(true);
}

void OctreeSystem::run_tick(bool only_recent) {
    y_profile();

    {
        y_profile_zone("updating moved objects");
        usize insertions = 0;
        for(auto&& [id, comp] : world().query<TransformableComponent>(transformable_ids(world(), only_recent))) {
            auto&& [tr] = comp;

            if(tr.local_aabb().is_empty()) {
                continue;
            }

            const AABB aabb = tr.global_aabb();

            if(tr._node) {
                if(tr._node->contains(aabb)) {
                    continue;
                }
                tr._node->remove(id);
            }

            tr._node = _tree.insert(id, aabb);
            ++insertions;
        }

        unused(insertions);
        y_profile_msg(fmt_c_str("%/% objects reinserted", insertions, transformable_ids(world(), only_recent).size()));
    }

    if(only_recent) {
        for(auto&& [id, comp] : world().query<ecs::Removed<TransformableComponent>>()) {
            auto&& [tr] = comp;

            if(tr._node) {
                tr._node->remove(id);
            }
        }
    }

    _tree.audit();
}

const OctreeNode& OctreeSystem::root() const {
    return _tree.root();
}

const Octree& OctreeSystem::octree() const {
    return _tree;
}

}

