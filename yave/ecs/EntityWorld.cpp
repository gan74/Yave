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

#include "EntityWorld.h"

#include <y/utils/log.h>
#include <y/utils/format.h>
#include <y/utils/memory.h>

#include <yave/assets/AssetLoadingContext.h>

#include <numeric>


namespace yave {
namespace ecs {

static auto create_component_containers() {
    y_profile();

    core::Vector<std::unique_ptr<ComponentContainerBase>> containers;
    for(const auto* poly_base = ComponentContainerBase::_y_serde3_poly_base.first; poly_base; poly_base = poly_base->next) {
        if(poly_base->create) {
            std::unique_ptr<ComponentContainerBase> container = poly_base->create();
            y_debug_assert(container);

            const ComponentTypeIndex id = container->type_id();
            containers.set_min_size(usize(id) + 1);
            containers[usize(id)] = std::move(container);
        }
    }
    return containers;
}

static EntityId create_prefab_entities(EntityWorld& world, const EntityPrefab& prefab, EntityIdMap& id_map, EntityId base_id = {}) {
    y_profile();

    y_debug_assert(prefab.original_id().is_valid());

    const EntityId id = base_id.is_valid() ? base_id : world.create_entity();
    y_always_assert(id_map.find(prefab.original_id()) == id_map.end(), "Invalid prefab: id is duplicated");
    id_map.emplace_back(prefab.original_id(), id);

    auto parent_child = [&](const auto& child) {
        if(child) {
            world.set_parent(create_prefab_entities(world, *child, id_map), id);
        }
    };

    for(const auto& child : prefab.children()) {
        parent_child(child);
    }

    for(const auto& child : prefab.asset_children()) {
        parent_child(child);
    }

    return id;
}

static void add_prefab_components(EntityWorld& world, const EntityPrefab& prefab, const EntityIdMap& id_map) {
    y_profile();

    y_debug_assert(prefab.original_id().is_valid());

    const auto it = id_map.find(prefab.original_id());
    y_debug_assert(it != id_map.end());

    for(const auto& comp : prefab.components()) {
        if(comp) {
            comp->add_to(world, it->second, id_map);
        }
    }

    auto add_child_components = [&](const auto& child) {
        if(child) {
            add_prefab_components(world, *child, id_map);
        }
    };

    for(const auto& child : prefab.children()) {
        add_child_components(child);
    }

    for(const auto& child : prefab.asset_children()) {
        add_child_components(child);
    }
}





EntityWorld::EntityWorld() : _containers(create_component_containers()) {
}

EntityWorld::~EntityWorld() {
    for(auto& system : _systems) {
        y_debug_assert(system->_world == this);
        system->destroy();
    }
    _containers.clear();
}

void EntityWorld::tick() {
    y_profile();

    {
        y_profile_zone("tick");
        for(auto& system : _systems) {
            y_profile_dyn_zone(system->name().data());
            y_debug_assert(system->_world == this);
            system->tick();
        }
    }

    {
        y_profile_zone("clean after tick");
        for(auto& container : _containers) {
            if(container) {
                container->_mutated.clear();
            }
        }
    }

    _entities.audit();
}

void EntityWorld::update(float dt) {
    y_profile();

    for(auto& system : _systems) {
        y_profile_dyn_zone(system->name().data());
        y_debug_assert(system->_world == this);
        system->update(dt);
        system->schedule_fixed_update(dt);
    }
}

usize EntityWorld::entity_count() const {
    return _entities.size();
}

bool EntityWorld::exists(EntityId id) const {
    return _entities.exists(id);
}

EntityId EntityWorld::create_entity() {
    y_profile();

    const EntityId id = _entities.create();
    _on_created.send(id);
    return id;
}

EntityId EntityWorld::create_entity(const EntityPrefab& prefab) {
    const EntityId id = create_entity();
    add_prefab(id, prefab);
    return id;
}


void EntityWorld::add_prefab(EntityId id, const EntityPrefab& prefab) {
    y_profile();

    EntityIdMap id_map;
    create_prefab_entities(*this, prefab, id_map, id);
    add_prefab_components(*this, prefab, id_map);
}

void EntityWorld::remove_entity(EntityId id) {
    y_profile();

    check_exists(id);

    _on_destroyed.send(id);

    remove_all_components(id);
    _entities.remove(id);

    // Entities are deleted immediatly, while components linger until the end of tick.
    // This needs to be changed to be made consistent.
    // Deferring entity deletion is problematic as we could add new components to the entity, while it is in limbo
    // Idealy deletions should be instantaneous, and we should rely on callbacks/events rather than to_be_removed & co
    Y_TODO(Fixme)
}

void EntityWorld::remove_all_components(EntityId id) {
    y_profile();

    for(auto& cont : _containers) {
        if(cont) {
            cont->remove(id);
        }
    }

    for(auto& [tag, container] : _tags) {
        unused(tag);
        if(container.contains(id)) {
            container.erase(id);
        }
    }
}

void EntityWorld::remove_all_entities() {
    y_profile();

    auto cached_entities = core::Vector<EntityId>::from_range(all_entities());
    for(const EntityId id : cached_entities) {
        remove_entity(id);
    }
}

EntityId EntityWorld::id_from_index(u32 index) const {
    return _entities.id_from_index(index);
}

EntityPrefab EntityWorld::create_prefab(EntityId id) const {
    check_exists(id);

    EntityPrefab prefab;
    y_fatal("not supported");
    return prefab;
}

EntityId EntityWorld::parent(EntityId id) const {
    return _entities.parent(id);
}

void EntityWorld::set_parent(EntityId id, EntityId parent_id) {
    y_profile();

    _entities.set_parent(id, parent_id);
}

bool EntityWorld::has_parent(EntityId id) const {
    return parent(id).is_valid();
}

bool EntityWorld::has_children(EntityId id) const {
    return _entities.first_child(id).is_valid();
}

bool EntityWorld::is_parent(EntityId id, EntityId parent) const {
    return _entities.is_parent(id, parent);
}

const SparseIdSetBase& EntityWorld::component_ids(ComponentTypeIndex type_id) const {
    return find_container(type_id)->id_set();
}

const SparseIdSet& EntityWorld::recently_mutated(ComponentTypeIndex type_id) const {
    return find_container(type_id)->recently_mutated();
}

core::Span<EntityId> EntityWorld::with_tag(const core::String& tag) const {
    const SparseIdSetBase* set = tag_set(tag);
    return set ? set->ids() : core::Span<EntityId>();
}

const SparseIdSet* EntityWorld::raw_tag_set(const core::String& tag) const {
    if(const auto it = _tags.find(tag); it != _tags.end()) {
        return &it->second;
    }
    return nullptr;
}

const SparseIdSetBase* EntityWorld::tag_set(const core::String& tag) const {
    if(tag.is_empty()) {
        return nullptr;
    }

    if(tag[0] == '@') {
        for(const auto& container : _containers) {
            if(container->runtime_info().clean_component_name() == tag.sub_str(1)) {
                return &container->id_set();
            }
        }
        return nullptr;
    }

    if(tag[0] == '!') {
        y_fatal("'!' tags can't have a set, use queries instead");
    }

    return raw_tag_set(tag);
}

void EntityWorld::add_tag(EntityId id, const core::String& tag) {
    check_exists(id);
    y_always_assert(!is_tag_implicit(tag), "Implicit tags can't be added directly");
    _tags[tag].insert(id);
}

void EntityWorld::remove_tag(EntityId id, const core::String& tag) {
    check_exists(id);
    y_always_assert(!is_tag_implicit(tag), "Implicit tags can't be removed directly");
    auto& tag_set = _tags[tag];
    if(tag_set.contains(id)) {
        tag_set.erase(id);
    }
}

void EntityWorld::clear_tag(const core::String& tag) {
    y_always_assert(!is_tag_implicit(tag), "Implicit tags can't be removed directly");
    _tags.erase(tag);
}

bool EntityWorld::has_tag(EntityId id, const core::String& tag) const {
    check_exists(id);
    const SparseIdSetBase* set = tag_set(tag);
    return set ? set->contains(id) : false;
}

bool EntityWorld::is_tag_implicit(std::string_view tag) {
    return !tag.empty() && (tag[0] == '@' || tag[0] == '!');
}

std::string_view EntityWorld::component_type_name(ComponentTypeIndex type_id) const {
    return find_container(type_id)->runtime_info().clean_component_name();
}

void EntityWorld::make_mutated(ComponentTypeIndex type_id, core::Span<EntityId> ids) {
    y_profile();
    auto& mutated = find_container(type_id)->_mutated;
    for(const EntityId id : ids) {
        mutated.insert(id);
    }
}

const ComponentContainerBase* EntityWorld::find_container(ComponentTypeIndex type_id) const {
    y_debug_assert(_containers.size() > usize(type_id));
    return _containers[usize(type_id)].get();
}

ComponentContainerBase* EntityWorld::find_container(ComponentTypeIndex type_id) {
    y_debug_assert(_containers.size() > usize(type_id));
    return _containers[usize(type_id)].get();
}

void EntityWorld::register_component_types(System* system) const {
    for(auto& container : _containers) {
        if(container) {
            container->register_component_type(system);
        }
    }
}

void EntityWorld::check_exists(EntityId id) const {
    y_always_assert(exists(id), "Entity doesn't exists");
}

void EntityWorld::inspect_components(EntityId id, ComponentInspector* inspector) {
    for(auto& container : _containers) {
        if(container) {
            container->inspect_component(id, inspector);
        }
    }
}


serde3::Result EntityWorld::save_state(serde3::WritableArchive& arc) const {
    y_profile();

    y_try(arc.serialize(_entities));
    y_try(arc.serialize(_tags));
    y_try(arc.serialize(_world_components));

    for(auto&& container : _containers) {
        if(container) {
            y_try(container->save_state(arc));
        }
    }

    return core::Ok(serde3::Success::Full);
}

serde3::Result EntityWorld::load_state(serde3::ReadableArchive& arc) {
    remove_all_entities();
    _tags.clear();
    _world_components.clear();

    y_try(arc.deserialize(_entities));
    y_try(arc.deserialize(_tags));
    y_try(arc.deserialize(_world_components));

    for(auto&& container : _containers) {
        if(container) {
            y_try(container->load_state(arc));
        }
    }


    {
        for(const EntityId id : _entities.ids()) {
            _on_created.send(id);
        }

        for(auto&& container : _containers) {
            if(container) {
                container->post_load();
            }
        }
    }

    return core::Ok(serde3::Success::Full);
}


}
}

