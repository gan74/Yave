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

#include "StaticMeshRendererSystem.h"

#include <yave/ecs/EntityWorld.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/descriptors/DescriptorSet.h>
#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>

#include <yave/material/Material.h>
#include <yave/meshes/StaticMesh.h>

#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>

namespace yave {


StaticMeshRendererSystem::StaticMeshRendererSystem() : ecs::System("StaticMeshRendererSystem"), _transforms(max_transforms) {
    _free.set_min_size(_transforms.size());
    std::iota(_free.begin(), _free.end(), 0);
}

void StaticMeshRendererSystem::destroy() {
    auto query = world().query<StaticMeshComponent>();
    for(auto&& [mesh] : query.components()) {
        free_index(mesh._transform_index);
        free_index(mesh._last_transform_index);
    }

    y_debug_assert(_free.size() == _transforms.size());
}

void StaticMeshRendererSystem::setup() {
    run_tick(false);
}

void StaticMeshRendererSystem::tick() {
    run_tick(true);
}

void StaticMeshRendererSystem::run_tick(bool only_recent) {
    auto moved_query = [&](auto query) {
        if(!query.size()) {
            return;
        }

        auto mapping = _transforms.map(MappingAccess::ReadWrite);
        for(const auto& [mesh, tr] : query.components()) {
            std::swap(mesh._last_transform_index, mesh._transform_index);
            alloc_index(mesh._transform_index);

            y_debug_assert(mesh.has_transform_index());
            mapping[mesh._transform_index] = tr.transform();
        }
    };

    if(only_recent) {
        moved_query(world().query<StaticMeshComponent, ecs::Changed<TransformableComponent>>());
    } else {
        moved_query(world().query<StaticMeshComponent, TransformableComponent>());
    }

    auto removed = world().query<ecs::Removed<StaticMeshComponent>>();
    for(const auto& [mesh] : removed.components()) {
        free_index(mesh._transform_index);
        free_index(mesh._last_transform_index);
    }
}

void StaticMeshRendererSystem::free_index(u32& index) {
    if(index != u32(-1)) {
        _free.push_back(index);
        index = u32(-1);
    }
}

void StaticMeshRendererSystem::alloc_index(u32& index) {
    if(index == u32(-1)) {
        y_always_assert(!_free.is_empty(), "Max number of transforms reached");
        index = _free.pop();
    }
}

}


