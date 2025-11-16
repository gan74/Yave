/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
void EcsScene::register_object(const ecs::EntityId id, u32 ObjectIndices::* index_ptr, S& storage) {
    u32& index = _indices.get_or_insert(id).*index_ptr;

    if(index == u32(-1)) {
        index = u32(storage.size());

        auto& obj = storage.emplace_back();
        y_debug_assert(obj.entity_index == u32(-1) || obj.entity_index == id.index());
        obj.entity_index = id.index();
    }
}

template<typename S>
u32 EcsScene::unregister_object(const ecs::EntityId id, u32 ObjectIndices::* index_ptr, S& storage) {
    ObjectIndices* object = _indices.try_get(id);
    if(!object) {
        return u32(-1);
    }

    const u32 index = std::exchange(object->*index_ptr, u32(-1));
    const u32 last_index = u32(storage.size() - 1);
    object->*index_ptr = u32(-1);

    y_debug_assert(id.is_valid());
    y_debug_assert(storage[index].entity_index == id.index());

    if(index != last_index) {
        const ecs::EntityId last_id = id_from_index(storage[last_index].entity_index);
        if(ObjectIndices* last_object = _indices.try_get(last_id)) {
            last_object->*index_ptr = index;
            std::swap(storage[index], storage[last_index]);
        }
    }

    if(object->is_empty()) {
        _indices.erase(id);
    }

    if constexpr(std::is_base_of_v<TransformableSceneObjectData, typename S::value_type>) {
        return storage.pop().transform_index;
    } else {
        storage.pop();
        return u32(-1);
    }
}

template<typename T, typename S>
void EcsScene::process_component_visibility(u32 ObjectIndices::* index_ptr, S& storage) {
    y_profile();

    auto update_visibility = [&](ecs::EntityId id, u32 mask) {
        const u32 index = _indices.try_get(id)->*index_ptr;
        if(index != u32(-1)) {
            storage[index].visibility_mask = mask;
        }
    };

    const std::array tag = {ecs::tags::hidden};
    const ecs::EntityGroupProvider* group_provider = _world->get_or_create_group_provider<T>(tag);
    y_debug_assert(group_provider->tags().size() == 1 && group_provider->tags()[0] == ecs::tags::hidden);

    for(const ecs::EntityId id : group_provider->added_ids()) {
        update_visibility(id, 0);
    }

    for(const ecs::EntityId id : group_provider->removed_ids()) {
        update_visibility(id, u32(-1));
    }
}

template<typename T, typename S>
bool EcsScene::process_transformable_components(u32 ObjectIndices::* index_ptr, S& storage) {
    y_profile();

    auto update_transform = [this](auto& obj, const TransformableComponent& tr, const auto& comp) {
        if(!obj.has_transform()) {
            obj.transform_index = _transform_manager.alloc_transform();
        }


        _transform_manager.set_transform(obj.transform_index, tr.transform());
        obj.global_aabb = tr.to_global(comp.aabb());
    };


    const ecs::EntityGroupProvider* group_provider = _world->get_or_create_group_provider<TransformableComponent, T>();

    {
        y_profile_zone("Add new objects");
        for(const ecs::EntityId id : group_provider->added_ids()) {
            register_object(id, index_ptr, storage);
        }
    }

    {
        y_profile_zone("Update components");
        auto group = _world->create_group<TransformableComponent, ecs::Changed<T>>();
        for(const auto& [id, tr, comp] : group.id_components()) {
            auto& obj = storage[_indices.try_get(id)->*index_ptr];

            obj.component = comp;
            // We need to update in case the AABB has changed
            update_transform(obj, tr, comp);
        }
    }

    {
        y_profile_zone("Update transforms");
        auto group = _world->create_group<ecs::Changed<TransformableComponent>, T>();
        for(const auto& [id, tr, comp] : group.id_components()) {
            auto& obj = storage[_indices.try_get(id)->*index_ptr];
            update_transform(obj, tr, comp);
        }
    }

    {
        y_profile_zone("Delete stale objects");
        for(const ecs::EntityId id : group_provider->removed_ids()) {
            if(const u32 transform_index = unregister_object(id, index_ptr, storage); transform_index == u32(-1)) {
                _transform_manager.free_transform(transform_index);
            }
        }
    }

    process_component_visibility<T>(index_ptr, storage);

    return !group_provider->removed_ids().is_empty();
}


template<typename T, typename S>
void EcsScene::process_components(u32 ObjectIndices::* index_ptr, S& storage) {
    y_profile();

    auto group = _world->create_group<ecs::Changed<T>>();
    const auto* group_base = group.base();

    {
        y_profile_zone("Add new objects");
        for(const ecs::EntityId id : group_base->added_ids()) {
            register_object(id, index_ptr, storage);
        }
    }

    {
        y_profile_zone("Update components");
        for(const auto& [id, comp] : group.id_components()) {
            auto& obj = storage[_indices.try_get(id)->*index_ptr];
            obj.component = comp;
        }
    }

    {
        y_profile_zone("Delete stale objects");
        for(const ecs::EntityId id : group_base->removed_ids()) {
            unregister_object(id, index_ptr, storage);
        }
    }

    process_component_visibility<T>(index_ptr, storage);
}

void EcsScene::process_atmosphere() {
    auto group = _world->create_group<AtmosphereComponent>();

    bool found = false;
    for(const auto& [id, atmo] : group.id_components()) {
        if(_world->has_tag(id, ecs::tags::hidden)) {
            continue;
        }
        if(const DirectionalLightComponent* sun = _world->component<DirectionalLightComponent>(atmo.sun())) {
            if(!_atmosphere) {
                _atmosphere = std::make_unique<AtmosphereObject>();
            }
            _atmosphere->entity_index = id.index();
            _atmosphere->component = atmo;
            _atmosphere->sun = *sun;
            found = true;
            break;
        }
    }

    if(!found) {
        _atmosphere = nullptr;
    }
}

const StaticMeshObject* EcsScene::mesh(ecs::EntityId id) const {
    if(const ObjectIndices* indices = _indices.try_get(id)) {
        if(indices->mesh != u32(-1)) {
            return &_meshes[indices->mesh];
        }
    }
    return nullptr;
}

const PointLightObject* EcsScene::point_light(ecs::EntityId id) const {
    if(const ObjectIndices* indices = _indices.try_get(id)) {
        if(indices->point_light != u32(-1)) {
            return &_point_lights[indices->point_light];
        }
    }
    return nullptr;
}

const SpotLightObject* EcsScene::spot_light(ecs::EntityId id) const {
    if(const ObjectIndices* indices = _indices.try_get(id)) {
        if(indices->spot_light != u32(-1)) {
            return &_spot_lights[indices->spot_light];
        }
    }
    return nullptr;
}

ecs::EntityId EcsScene::id_from_index(u32 index) const {
    return _indices.id_from_index(index);
}

void EcsScene::update_from_world() {
    y_profile();

    y_debug_assert(_world);


    bool need_tlas_rebuild = _tlas.is_null();
    need_tlas_rebuild |= process_transformable_components<StaticMeshComponent>(&ObjectIndices::mesh, _meshes);

    process_transformable_components<PointLightComponent>(&ObjectIndices::point_light, _point_lights);
    process_transformable_components<SpotLightComponent>(&ObjectIndices::spot_light, _spot_lights);

    process_components<DirectionalLightComponent>(&ObjectIndices::directional_light, _directionals);
    process_components<SkyLightComponent>(&ObjectIndices::sky_light, _sky_lights);

    process_atmosphere();

    if(_transform_manager.need_update()) {
        ComputeCmdBufferRecorder recorder = create_disposable_compute_cmd_buffer();
        _transform_manager.update_buffer(recorder);
        recorder.submit_async();
        need_tlas_rebuild |= true;
    }

    if(need_tlas_rebuild) {
       update_tlas();
    }

    audit();
}

void EcsScene::audit() const {
#ifdef Y_DEBUG
    y_profile();

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

