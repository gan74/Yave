/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include "AssetLoaderSystem.h"

#include <yave/components/StaticMeshComponent.h>
#include <yave/components/SkyLightComponent.h>

#include <yave/material/Material.h>
#include <yave/material/SimpleMaterialData.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/meshes/MeshData.h>

#include <yave/graphics/images/IBLProbe.h>
#include <yave/graphics/images/ImageData.h>

#include <yave/assets/AssetLoader.h>
#include <yave/assets/AssetPtr.h>
#include <yave/ecs/EntityWorld.h>

namespace yave {

template<typename T>
static core::Span<ecs::EntityId> ids(ecs::EntityWorld& world, bool recent) {
    return recent
        ? world.recently_added<T>()
        : world.component_ids<T>();
}


AssetLoaderSystem::AssetLoaderSystem(AssetLoader& loader) : ecs::System("AssetLoaderSystem"), _loader(&loader) {
}

void AssetLoaderSystem::setup(ecs::EntityWorld& world) {
    run_tick(world, false);
}

void AssetLoaderSystem::tick(ecs::EntityWorld& world) {
    run_tick(world, true);
}

void AssetLoaderSystem::run_tick(ecs::EntityWorld& world, bool only_recent) {
    AssetLoadingContext loading_ctx(_loader);

    for(ecs::EntityId id : ids<StaticMeshComponent>(world, only_recent)) {
        StaticMeshComponent* component = world.component<StaticMeshComponent>(id);
        for(auto& sub_mesh : component->sub_meshes()) {
            sub_mesh.mesh.load(loading_ctx);
            sub_mesh.material.load(loading_ctx);
        }
    }

    for(ecs::EntityId id : ids<SkyLightComponent>(world, only_recent)) {
        SkyLightComponent* component = world.component<SkyLightComponent>(id);
        component->probe().load(loading_ctx);
    }
}



}
