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

#include "StaticMeshManagerSystem.h"

#include <yave/ecs/EntityWorld.h>

#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>

namespace yave {

StaticMeshManagerSystem::StaticMeshManagerSystem() : ecs::System("StaticMeshManagerSystem"), _transforms(max_transforms) {
    _free.set_min_size(_transforms.size());
    std::iota(_free.begin(), _free.end(), 0);
}

void StaticMeshManagerSystem::destroy(ecs::EntityWorld& world) {
    auto query = world.query<ecs::Mutate<StaticMeshComponent>>();
    for(auto&& [mesh] : query.components()) {
        if(mesh.has_instance_index()) {
            _free.push_back(mesh._instance_index);
            mesh._instance_index = u32(-1);
        }
    }
}

void StaticMeshManagerSystem::setup(ecs::EntityWorld& world) {
    run_tick(world, false);
}

void StaticMeshManagerSystem::tick(ecs::EntityWorld& world) {
    run_tick(world, true);
}

void StaticMeshManagerSystem::run_tick(ecs::EntityWorld& world, bool only_recent) {
    auto moved_query = [&](auto query) {
        if(!query.size()) {
            return;
        }

        auto mapping = _transforms.map(MappingAccess::ReadWrite);
        for(const auto& [mesh, tr] : query.components()) {
            if(!mesh.has_instance_index()) {
                y_always_assert(!_free.is_empty(), "Max number of transform reached");
                mesh._instance_index = _free.pop();
            }

            y_debug_assert(mesh.has_instance_index());
            mapping[mesh._instance_index] = tr.transform();
        }
    };

    if(only_recent) {
        moved_query(world.query<StaticMeshComponent, ecs::Changed<TransformableComponent>>());
    } else {
        moved_query(world.query<StaticMeshComponent, TransformableComponent>());
    }
}

}


