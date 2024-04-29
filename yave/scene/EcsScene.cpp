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

#include "EcsScene.h"

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

EcsScene::EcsScene(const ecs::EntityWorld* world) : _world(world) {
}

void EcsScene::update_from_world() {
    y_profile();

    y_debug_assert(_world);

    {
        auto query = _world->query<TransformableComponent, StaticMeshComponent>();
        for(const auto& [id, tr, mesh] : query.id_components()) {
            if(!mesh._scene_repr) {
                mesh._scene_repr = &_meshes.emplace();
                mesh._scene_repr->entity_index = id.index();
            }

            if(mesh._scene_repr->transform_index == u32(-1)) {
                mesh._scene_repr->transform_index = _transform_manager.alloc_transform();
            }

            _transform_manager.set_transform(mesh._scene_repr->transform_index, tr.transform());

            mesh._scene_repr->mesh = mesh.mesh();
            mesh._scene_repr->materials = mesh.materials();
        }
    }

    {
        ComputeCmdBufferRecorder recorder = create_disposable_compute_cmd_buffer();
        _transform_manager.update_buffer(recorder);
        recorder.submit_async();
    }
}

}

