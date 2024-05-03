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

EcsScene::EcsScene(const ecs::EntityWorld* w) : _world(w) {
    update_from_world();
}

const ecs::EntityWorld* EcsScene::world() const {
    return _world;
}

template<typename S>
typename S::value_type& EcsScene::register_object(u32& index, S& storage) {
    using object_t = typename S::value_type;

    object_t* obj = nullptr;
    if(index == u32(-1)) {
        index = u32(storage.size());
        obj = &storage.emplace_back();
    } else {
        obj = &storage[index];
    }

    return *obj;
}

template<typename S>
void EcsScene::process_transformable_components(u32 ObjectIndices::* index, S& storage) {
    using component_t = std::tuple_element_t<1, typename S::value_type>;

    auto query = _world->query<TransformableComponent, component_t>();
    for(const auto& [id, tr, comp] : query.id_components()) {
        auto& obj = register_object(_indices.get_or_insert(id).*index, storage);
        std::get<component_t>(obj) = comp;

        auto& tr_obj = std::get<TransformableSceneObject>(obj);
        if(tr_obj.transform_index == u32(-1)) {
            tr_obj.transform_index = _transform_manager.alloc_transform();
        }

        _transform_manager.set_transform(tr_obj.transform_index, tr.transform());
        tr_obj.global_aabb = tr.to_global(comp.aabb());
        tr_obj.id = id;
    }
}


template<typename S>
void EcsScene::process_components(u32 ObjectIndices::* index, S& storage) {
    using component_t = std::tuple_element_t<1, typename S::value_type>;

    auto query = _world->query<component_t>();
    for(const auto& [id, comp] : query.id_components()) {
        auto& obj = register_object(_indices.get_or_insert(id).*index, storage);
        std::get<component_t>(obj) = comp;

        std::get<SceneObject>(obj).id = id;
    }
}

void EcsScene::update_from_world() {
    y_profile();

    y_debug_assert(_world);

    process_transformable_components(&ObjectIndices::mesh, _meshes);
    process_transformable_components(&ObjectIndices::point_light, _point_lights);
    process_transformable_components(&ObjectIndices::spot_light, _spot_lights);
    process_components(&ObjectIndices::directional_light, _directionals);
    process_components(&ObjectIndices::sky_light, _sky_lights);


    {
        ComputeCmdBufferRecorder recorder = create_disposable_compute_cmd_buffer();
        _transform_manager.update_buffer(recorder);
        recorder.submit_async();
    }
}

}

