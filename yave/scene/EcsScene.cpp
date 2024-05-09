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
typename S::value_type& EcsScene::register_object(const ecs::EntityId id, u32 ObjectIndices::* index_ptr, S& storage) {
    using object_t = typename S::value_type;

    u32& index = _indices.get_or_insert(id).*index_ptr;

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
typename S::value_type EcsScene::unregister_object(const ecs::EntityId id, u32 ObjectIndices::* index_ptr, S& storage) {
    auto& object = _indices[id];
    const u32 index = std::exchange(object.*index_ptr, u32(-1));
    const u32 last_index = u32(storage.size() - 1);
    object.*index_ptr = u32(-1);

    y_debug_assert(id.is_valid());
    y_debug_assert(storage[index].entity_index == id.index());

    if(index != last_index) {
        const ecs::EntityId last_id = id_from_index(storage[last_index].entity_index);
        _indices[last_id].*index_ptr = index;
        std::swap(storage[index], storage[last_index]);
    }

    _indices.erase(id);

    return storage.pop();
}

template<typename T, typename S>
void EcsScene::process_transformable_components(u32 ObjectIndices::* index_ptr, S& storage) {
    y_profile();

    {
        y_profile_zone("Update components");
        auto query = _world->create_group<TransformableComponent, ecs::Changed<T>>().query();
        for(const auto& [id, tr, comp] : query.id_components()) {
            auto& obj = register_object(id, index_ptr, storage);

            obj.component = comp;
            obj.entity_index = id.index();
        }
    }

    {
        y_profile_zone("Update transforms");
        auto query = _world->create_group<ecs::Changed<TransformableComponent>, T>().query();
        for(const auto& [id, tr, comp] : query.id_components()) {
            auto& obj = register_object(id, index_ptr, storage);

            if(obj.transform_index == u32(-1)) {
                obj.transform_index = _transform_manager.alloc_transform();
            }

            _transform_manager.set_transform(obj.transform_index, tr.transform());
            obj.global_aabb = tr.to_global(comp.aabb());
        }
    }

    {
        y_profile_zone("Delete stale objects");
        auto query = _world->create_group<ecs::Deleted<T>>().query();
        if(query.size())
        log_msg(fmt("{} deleted {}", query.size(), ct_type_name<T>()));
        for(const ecs::EntityId id : query.ids()) {
            if(const u32 transform_index = unregister_object(id, index_ptr, storage).transform_index; transform_index == u32(-1)) {
                _transform_manager.free_transform(transform_index);
            }
        }
    }
}


template<typename T, typename S>
void EcsScene::process_components(u32 ObjectIndices::* index_ptr, S& storage) {
    y_profile();

    {
        auto query = _world->create_group<ecs::Changed<T>>().query();
        for(const auto& [id, comp] : query.id_components()) {
            auto& obj = register_object(id, index_ptr, storage);
            obj.component = comp;
            obj.entity_index = id.index();
        }
    }

    {
        auto query = _world->create_group<ecs::Deleted<T>>().query();
        for(const ecs::EntityId id : query.ids()) {
             unregister_object(id, index_ptr, storage);
        }
    }
}

void EcsScene::process_atmosphere() {
    auto query = _world->create_group<AtmosphereComponent>().query();
    if(query.is_empty()) {
        _atmosphere = nullptr;
    } else {
        for(const auto& [id, atmo] : query.id_components()) {
            const DirectionalLightComponent* sun = _world->component<DirectionalLightComponent>(atmo.sun());

            if(sun) {
                if(!_atmosphere) {
                    _atmosphere = std::make_unique<AtmosphereObject>();
                }
                _atmosphere->entity_index = id.index();
                _atmosphere->component = atmo;
                _atmosphere->sun = *sun;
                break;
            }
        }
    }
}

ecs::EntityId EcsScene::id_from_index(u32 index) const {
    return _indices.id_from_index(index);
}

void EcsScene::update_from_world() {
    y_profile();

    y_debug_assert(_world);

    process_transformable_components<StaticMeshComponent>(&ObjectIndices::mesh, _meshes);
    process_transformable_components<PointLightComponent>(&ObjectIndices::point_light, _point_lights);
    process_transformable_components<SpotLightComponent>(&ObjectIndices::spot_light, _spot_lights);
    process_components<DirectionalLightComponent>(&ObjectIndices::directional_light, _directionals);
    process_components<SkyLightComponent>(&ObjectIndices::sky_light, _sky_lights);

    process_atmosphere();

    {
        ComputeCmdBufferRecorder recorder = create_disposable_compute_cmd_buffer();
        _transform_manager.update_buffer(recorder);
        recorder.submit_async();
    }


    audit();
}

void EcsScene::audit() const {
#ifdef Y_DEBUG
    for(const auto& [id, object] : _indices) {
        if(object.mesh != u32(-1)) {
            y_debug_assert(_meshes[object.mesh].entity_index == id.index());
        }
    }

    for(usize i = 0; i != _meshes.size(); ++i) {
        const ecs::EntityId id = id_from_index(_meshes[i].entity_index);
        y_debug_assert(id.is_valid());
        y_debug_assert(_indices[id].mesh == u32(i));
    }
#endif
}

}

