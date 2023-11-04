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
#include <yave/camera/Camera.h>

#include <yave/ecs/EntityWorld.h>

#include <y/core/Chrono.h>

namespace yave {


OctreeSystem::OctreeSystem() : ecs::System("OctreeSystem") {
}

void OctreeSystem::destroy() {
    auto query = world().query<TransformableComponent>();
    for(auto&& [tr] : query.components()) {
        tr._node = nullptr;
    }
}

void OctreeSystem::setup() {
    _transform_destroyed = world().on_destroyed<TransformableComponent>().subscribe([](ecs::EntityId id, TransformableComponent& tr) {
        if(tr._node) {
            tr._node->remove(id);
        }
    });

    run_tick(false);
}

void OctreeSystem::tick() {
    run_tick(true);
}

void OctreeSystem::run_tick(bool only_recent) {
    y_profile();

    auto query = only_recent
        ? world().query<ecs::Changed<TransformableComponent>>()
        : world().query<TransformableComponent>();

    usize insertions = 0;
    for(auto&& [id, comp] : query) {
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
    y_profile_msg(fmt_c_str("{}/{} objects inserted", insertions, query.size()));

    _tree.audit();
}

core::Vector<ecs::EntityId> OctreeSystem::find_entities(const Camera& camera) const {
    auto visible = _tree.find_entities(camera.frustum(), camera.far_plane_dist());

    core::Vector<ecs::EntityId> entities;
    entities.swap(visible.inside);

    Y_TODO(do intersection tests)
    entities.push_back(visible.intersect.begin(), visible.intersect.end());

    return entities;
}

const OctreeNode& OctreeSystem::root() const {
    return _tree.root();
}

}

