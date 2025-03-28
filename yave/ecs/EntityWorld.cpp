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

            const usize index = usize(container->type_id());
            containers.set_min_size(index + 1);

            y_debug_assert(!containers[index]);
            containers[index] = std::move(container);
        } else {
            log_msg(fmt("Could not create '{}'", poly_base->name), Log::Warning);
        }
    }
    return containers;
}

static EntityId create_prefab_entities(EntityWorld& world, const EntityPrefab& prefab, EntityIdMap& id_map, bool keep_ids, EntityId base_id = {}) {
    y_profile();

    y_debug_assert(prefab.original_id().is_valid());

    const EntityId id =
        base_id.is_valid()
        ? base_id
        : keep_ids ? world.create_entity_with_id(prefab.original_id()) : world.create_entity()
    ;


    y_always_assert(id_map.find(prefab.original_id()) == id_map.end(), "Invalid prefab: id is duplicated");
    id_map.emplace_back(prefab.original_id(), id);

    auto parent_child = [&](const auto& child) {
        if(child) {
            world.set_parent(create_prefab_entities(world, *child, id_map, keep_ids), id);
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
    register_containers();
}

EntityWorld::~EntityWorld() {
}

TickId EntityWorld::tick_id() const {
    return _tick_id;
}

void EntityWorld::tick(concurrent::StaticThreadPool& thread_pool) {
    _tick_id = _tick_id.next();
    _system_manager.run_schedule_seq();
    _system_manager.run_schedule_mt(thread_pool);
}

void EntityWorld::process_deferred_changes() {
    _groups.locked([&](auto&& groups) {
        y_profile_zone("Clear removed groups");
        for(auto& group : groups) {
            group->_removed.make_empty();
        }
    });

    for(auto& container : _ordered_containers) {
        container->process_deferred_changes();
    }

    for(const EntityId id : _to_delete) {
        _matrix.remove_entity(id);
        _entities.remove(id);
    }

    _to_delete.make_empty();
    _recently_added.make_empty();
    _parent_changed.make_empty();

    _groups.locked([&](auto&& groups) {
        y_profile_zone("Clear added groups");
        for(auto& group : groups) {
            group->_added.make_empty();
        }
    });

    _entities.audit();
}

std::string_view EntityWorld::component_type_name(ComponentTypeIndex type_id) const {
    return find_container(type_id)->runtime_info().clean_component_name();
}

core::Vector<const EntityGroupProvider*> EntityWorld::group_providers() {
    core::Vector<const EntityGroupProvider*> gr;
    _groups.locked([&](auto&& groups) {
        std::transform(groups.begin(), groups.end(), std::back_inserter(gr), [](const auto& g) { return g.get(); });
    });
    return gr;
}

core::Span<ComponentContainerBase*> EntityWorld::component_containers() const {
    return _ordered_containers;
}

usize EntityWorld::entity_count() const {
    return _entities.size();
}

bool EntityWorld::exists(EntityId id) const {
    return _entities.exists(id);
}

const SparseIdSet& EntityWorld::pending_deletions() const {
    return _to_delete;
}

core::Span<EntityId> EntityWorld::recently_added() const {
    return _recently_added;
}

const SparseIdSet& EntityWorld::parent_changed() const {
    return _parent_changed;
}

EntityId EntityWorld::create_entity() {
    const EntityId id = _entities.create();
    _matrix.add_entity(id);
    _recently_added << id;
    return id;
}

EntityId EntityWorld::create_entity_with_id(EntityId id) {
    y_debug_assert(!exists(id));
    y_always_assert(_entities.create_with_id(id).is_valid(), "Entity ID already in use");
    _matrix.add_entity(id);
    _recently_added << id;
    return id;
}

EntityId EntityWorld::create_entity(const EntityPrefab& prefab, bool keep_ids) {
    const EntityId id = keep_ids ? create_entity_with_id(prefab.original_id()) : create_entity();
    add_prefab(id, prefab, keep_ids);
    return id;
}

void EntityWorld::add_prefab(EntityId id, const EntityPrefab& prefab, bool keep_ids) {
    y_profile();

    EntityIdMap id_map;
    create_prefab_entities(*this, prefab, id_map, keep_ids, id);
    add_prefab_components(*this, prefab, id_map);
}

EntityPrefab EntityWorld::create_prefab_from_entity(EntityId id) const {
    y_profile();

    check_exists(id);

    EntityPrefab prefab(id);

    for(const auto& container : _containers) {
        if(container) {
            if(auto box = container->create_box(id)) {
                prefab.add_box(std::move(box));
            }
        }
    }

    for(const EntityId child : children(id)) {
        prefab.add_child(std::make_unique<EntityPrefab>(create_prefab_from_entity(child)));
    }

    return prefab;
}

std::unique_ptr<ComponentBoxBase> EntityWorld::create_box_from_component(EntityId id, ComponentTypeIndex type_id) const {
    y_profile();

    if(auto container = find_container(type_id)) {
        return container->create_box(id);
    }

    return nullptr;
}

void EntityWorld::remove_entity(EntityId id) {
    if(_to_delete.insert(id)) {
        remove_all_components(id);
        remove_all_tags(id);
    }
}

void EntityWorld::remove_all_components(EntityId id) {
    y_profile();

    for(auto& container : _ordered_containers) {
        container->remove_later(id);
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
    y_debug_assert(!is_computed_tag(tag));
    _matrix.add_tag(id, tag);
}

void EntityWorld::remove_tag(EntityId id, const core::String& tag) {
    y_debug_assert(exists(id));
    y_debug_assert(!is_computed_tag(tag));
    _matrix.remove_tag(id, tag);
}

void EntityWorld::clear_tag(const core::String& tag) {
    y_debug_assert(!is_computed_tag(tag));
    _matrix.clear_tag(tag);
}

bool EntityWorld::has_tag(EntityId id, const core::String& tag) const {
    y_debug_assert(exists(id));
    y_debug_assert(!is_computed_tag(tag));
    return _matrix.has_tag(id, tag);
}

const SparseIdSet* EntityWorld::tag_set(const core::String& tag) const {
    y_debug_assert(!is_computed_tag(tag));
    return _matrix.tag_set(tag);
}

EntityId EntityWorld::parent(EntityId id) const {
    return _entities.parent(id);
}

void EntityWorld::set_parent(EntityId id, EntityId parent_id) {
    y_profile();

    _entities.set_parent(id, parent_id);
    _parent_changed.insert(id);
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

bool EntityWorld::is_component_required(EntityId id, ComponentTypeIndex type) const {
    return find_container(type)->is_component_required(id);
}

void EntityWorld::remove_component(EntityId id, ComponentTypeIndex type) {
    return find_container(type)->remove_later(id);
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

void EntityWorld::inspect_components(EntityId id, ComponentInspector* inspector, ComponentTypeIndex type_id) {
    for(auto& container : _containers) {
        if(container) {
            if(type_id == ComponentTypeIndex::invalid_index || container->type_id() == type_id) {
                container->inspect_component(id, inspector);
            }
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

void EntityWorld::register_containers() {
    y_profile();

    _ordered_containers.clear();
    for(auto&& container : _containers) {
        if(!container) {
            continue;
        }


        _ordered_containers << container.get();


        container->_matrix = &_matrix;


        {
            const core::Span required_types = container->runtime_info().required;

            container->_required = core::FixedArray<ComponentContainerBase*>(required_types.size());
            for(usize i = 0; i != required_types.size(); ++i) {
                ComponentContainerBase* req_container = find_container(required_types[i]);
                container->_required[i] = req_container;
            }

            y_debug_assert(std::all_of(container->_required.begin(), container->_required.end(), [](auto* p) { return !!p; }));

            for(ComponentContainerBase* req_container : container->_required) {
                req_container->_required_by << container->type_id();
            }
        }
    }

    {
        y_profile_zone("sort containers");
        std::sort(_ordered_containers.begin(), _ordered_containers.end(), [](const auto* a, const auto* b) { return a->requirements_chain_depth() > b->requirements_chain_depth(); });
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
    y_profile();

    y_always_assert(!entity_count(), "World should be empty before loading");

    decltype(_containers) containers;

    y_try(arc.deserialize(_entities));
    y_try(arc.deserialize(containers));
    y_try(_matrix.load_tags(arc));

    for(EntityId id : _entities.ids()) {
        _matrix.add_entity(id);
    }

    _containers = create_component_containers();
    for(auto&& container : containers) {
        if(container) {
            const usize index = usize(container->type_id());
            _containers.set_min_size(index + 1);
            _containers[index] = std::move(container);
        }
    }

    register_containers();

    for(auto&& container : _containers) {
        if(container) {
            container->post_load();
        }
    }

    _system_manager.reset();

    return core::Ok(serde3::Success::Full);
}


}
}

