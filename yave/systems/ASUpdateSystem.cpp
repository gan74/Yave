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

#include "ASUpdateSystem.h"

#include <yave/components/RayTracingComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/ecs/EntityWorld.h>

namespace yave {

ASUpdateSystem::ASUpdateSystem() : ecs::System("ASUpdateSystem") {
}

void ASUpdateSystem::setup(ecs::EntityWorld& world) {
    run_tick(world, false);
}

void ASUpdateSystem::tick(ecs::EntityWorld& world) {
    run_tick(world, true);
}

void ASUpdateSystem::run_tick(ecs::EntityWorld& world, bool only_recent) {
    const RayTracing* rt = ray_tracing();
    if(!rt) {
        return;
    }

    const core::Span<ecs::EntityId> rt_ids = only_recent
        ? world.recently_added<RayTracingComponent>()
        : world.component_ids<RayTracingComponent>();

    _to_update.push_back(rt_ids.begin(), rt_ids.end());

    auto to_keep = core::vector_with_capacity<ecs::EntityId>(_to_update.size());
    for(auto&& id_comp : world.query<ecs::Mutate<RayTracingComponent>, StaticMeshComponent>(_to_update).id_components()) {
        const StaticMeshComponent& mesh = id_comp.component<StaticMeshComponent>();
        if(!mesh.is_fully_loaded()) {
            to_keep << id_comp.id();
            continue;
        }

        id_comp.component<RayTracingComponent>() = RayTracingComponent(mesh);
    }

    _to_update.swap(to_keep);

}

}

