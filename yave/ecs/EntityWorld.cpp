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




EntityWorld::EntityWorld() : _containers(create_component_containers()), _matrix(_containers.size()), _system_manager(this) {
    for(auto& container : _containers) {
        if(container) {
            container->_matrix = &_matrix;
        }
    }
}

EntityWorld::~EntityWorld() {
    _containers.clear();
}

void EntityWorld::tick(concurrent::StaticThreadPool& thread_pool) {
    _system_manager.run_schedule(thread_pool);
}

void EntityWorld::process_deferred_changes() {
    for(auto& container : _containers) {
        if(container) {
            container->process_deferred_changes();
        }
    }

    for(const EntityId id : _to_delete) {
        _matrix.remove_entity(id);
        _entities.remove(id);
    }
    _to_delete.make_empty();
}

std::string_view EntityWorld::component_type_name(ComponentTypeIndex type_id) const {
    return find_container(type_id)->runtime_info().clean_component_name();
}

core::Vector<const EntityGroupBase*> EntityWorld::all_group_bases() {
    core::Vector<const EntityGroupBase*> gr;
    _groups.locked([&](auto&& groups) {
        std::transform(groups.begin(), groups.end(), std::back_inserter(gr), [](const auto& g) { return g.get(); });
    });
    return gr;
}

usize EntityWorld::entity_count() const {
    return _entities.size();
}

bool EntityWorld::exists(EntityId id) const {
    return _entities.exists(id);
}

EntityId EntityWorld::create_entity() {
    const EntityId id = _entities.create();
    _matrix.add_entity(id);
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

void EntityWorld::clear_immediate() {
    remove_all_entities();
    process_deferred_changes();

    _matrix.clear();
    _groups.locked([](auto&& groups) { groups.clear(); });
}

void EntityWorld::remove_entity(EntityId id) {
    if(_to_delete.insert(id)) {
        remove_all_components(id);
        remove_all_tags(id);
    }
}

void EntityWorld::remove_all_components(EntityId id) {
    y_profile();

    for(auto& container : _containers) {
        if(container) {
            container->remove_later(id);
        }
    }
}

void EntityWorld::remove_all_tags(EntityId id) {
    y_profile();

    for(const core::String& tag : _matrix.tags()) {
        _matrix.remove_tag(id, tag);
    }
}

void EntityWorld::remove_all_entities() {
    auto cached_entities = core::Vector<EntityId>::from_range(_entities.ids());
    for(const EntityId id : cached_entities) {
        remove_entity(id);
    }
}

const EntityPool& EntityWorld::entity_pool() const {
    return _entities;
}

void EntityWorld::add_tag(EntityId id, const core::String& tag) {
    y_debug_assert(exists(id));
    y_debug_assert(!is_tag_implicit(tag));
    _matrix.add_tag(id, tag);
}

void EntityWorld::remove_tag(EntityId id, const core::String& tag) {
    y_debug_assert(exists(id));
    y_debug_assert(!is_tag_implicit(tag));
    _matrix.remove_tag(id, tag);
}

void EntityWorld::clear_tag(const core::String& tag) {
    y_debug_assert(!is_tag_implicit(tag));
    _matrix.clear_tag(tag);
}

bool EntityWorld::has_tag(EntityId id, const core::String& tag) const {
    y_debug_assert(exists(id));
    y_debug_assert(!is_tag_implicit(tag));
    return _matrix.has_tag(id, tag);
}

const SparseIdSet* EntityWorld::tag_set(const core::String& tag) const {
    y_debug_assert(!is_tag_implicit(tag));
    return _matrix.tag_set(tag);
}

bool EntityWorld::is_tag_implicit(std::string_view tag) {
    return !tag.empty() && (tag[0] == '@' || tag[0] == '!');
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

bool EntityWorld::has_component(EntityId id, ComponentTypeIndex type) const {
    y_debug_assert(exists(id));
    return _matrix.type_exists(type) && _matrix.has_component(id, type);
}

const ComponentContainerBase* EntityWorld::find_container(ComponentTypeIndex type_id) const {
    y_debug_assert(_containers.size() > usize(type_id));
    return _containers[usize(type_id)].get();
}

ComponentContainerBase* EntityWorld::find_container(ComponentTypeIndex type_id) {
    y_debug_assert(_containers.size() > usize(type_id));
    return _containers[usize(type_id)].get();
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

void EntityWorld::register_component_types(System* system) const {
    for(auto& container : _containers) {
        if(container) {
            container->register_component_type(system);
        }
    }
}

serde3::Result EntityWorld::save_state(serde3::WritableArchive& arc) const {
    y_profile();

    y_try(arc.serialize(_entities));
    y_try(arc.serialize(_containers));
    y_try(_matrix.save_tags(arc));

    return core::Ok(serde3::Success::Full);
}

serde3::Result EntityWorld::load_state(serde3::ReadableArchive& arc) {
    clear_immediate();

    decltype(_containers) containers;

    y_try(arc.deserialize(_entities));
    y_try(arc.deserialize(containers));
    y_try(_matrix.load_tags(arc));

    for(EntityId id : _entities.ids()) {
        _matrix.add_entity(id);
    }

    for(auto&& container : containers) {
        if(container) {
            container->_matrix = &_matrix;
            container->post_load();

            _containers[usize(container->type_id())] = std::move(container);
        }
    }

    return core::Ok(serde3::Success::Full);
}


}
}

