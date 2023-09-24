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

EntityWorld::EntityWorld() : _containers(create_component_containers()) {
}

EntityWorld::~EntityWorld() {
    for(auto& system : _systems) {
        y_debug_assert(system->_world == this);
        system->destroy();
    }
    _containers.clear();
}

EntityWorld::EntityWorld(EntityWorld&& other) {
    swap(other);
}

EntityWorld& EntityWorld::operator=(EntityWorld&& other) {
    swap(other);
    return *this;
}

void EntityWorld::swap(EntityWorld& other) {
    if(this != &other) {
        std::swap(_containers, other._containers);
        std::swap(_tags, other._tags);
        std::swap(_entities, other._entities);
        std::swap(_required_components, other._required_components);
        std::swap(_systems, other._systems);
        std::swap(_world_components, other._world_components);

        for(auto& system : _systems) {
            y_debug_assert(system->_world == &other);
            system->_world = this;
        }

        for(auto& system : other._systems) {
            y_debug_assert(system->_world == this);
            system->_world = &other;
        }
    }
    for(const ComponentTypeIndex c : _required_components) {
        unused(c);
        y_debug_assert(find_container(c));
    }
}

void EntityWorld::tick() {
    y_profile();

    {
        y_profile_zone("prepare tick");
        for(auto& container : _containers) {
            if(container) {
                container->prepare_for_tick();
            }
        }
    }

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
                container->clean_after_tick();
            }
        }
    }
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
    return _entities.contains(id);
}

EntityId EntityWorld::create_entity() {
    const EntityId id = _entities.create();
    for(const ComponentTypeIndex c : _required_components) {
        ComponentContainerBase* container = find_container(c);
        y_debug_assert(container && container->type_id() == c);
        container->add(*this, id);
    }
    return id;
}

EntityId EntityWorld::create_entity(const Archetype& archetype) {
    const EntityId id = create_entity();
    for(const auto& info : archetype.component_infos()) {
        ComponentContainerBase* container = find_container(info.type_id);
        y_debug_assert(container);
        container->add(*this, id);
    }
    return id;
}

EntityId EntityWorld::create_entity(const EntityPrefab& prefab) {
    const EntityId id = create_entity();
    for(const auto& comp : prefab.components()) {
        if(!comp) {
            log_msg("Unable to add null component", Log::Error);
        } else {
            comp->add_to(*this, id);
        }
    }
    return id;
}

void EntityWorld::remove_entity(EntityId id) {
    check_exists(id);
    for(auto& container : _containers) {
        if(container) {
            container->remove(id);
        }
    }
    for(auto& [tag, container] : _tags) {
        unused(tag);
        if(container.contains(id)) {
            container.erase(id);
        }
    }
    _entities.recycle(id);
}

EntityId EntityWorld::id_from_index(u32 index) const {
    return _entities.id_from_index(index);
}

EntityPrefab EntityWorld::create_prefab(EntityId id) const {
    check_exists(id);
    EntityPrefab prefab;
    for(auto& container : _containers) {
        if(container) {
            if(!container->contains(id)) {
                continue;
            }
            auto box = container->create_box(id);
            if(!box) {
                log_msg(fmt("{} is not copyable and was excluded from prefab", container->runtime_info().type_name), Log::Warning);
            }
            prefab.add(std::move(box));
        }
    }
    return prefab;
}

core::Span<EntityId> EntityWorld::component_ids(ComponentTypeIndex type_id) const {
    return find_container(type_id)->ids();
}

core::Span<EntityId> EntityWorld::recently_mutated(ComponentTypeIndex type_id) const {
    return find_container(type_id)->recently_mutated().ids();
}

core::Span<EntityId> EntityWorld::to_be_removed(ComponentTypeIndex type_id) const {
    return find_container(type_id)->to_be_removed().ids();
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

core::Span<ComponentTypeIndex> EntityWorld::required_components() const {
    return _required_components;
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

void EntityWorld::post_deserialize() {
    y_profile();

    auto patched = create_component_containers();
    for(auto& container : _containers) {
        if(!container) {
            continue;
        }

        container->_to_remove.clear();
        container->_mutated.clear();
        for(const EntityId id : container->ids()) {
            container->_mutated.insert(id);
        }

        const ComponentTypeIndex id = container->type_id();
        patched.set_min_size(usize(id) + 1);
        patched[usize(id)] = std::move(container);
    }
    _containers = std::move(patched);

    for(auto& system : _systems) {
        y_debug_assert(system);
        y_debug_assert(system->_world == this);
        system->reset();
    }


    for(const ComponentTypeIndex c : _required_components) {
        unused(c);
        y_always_assert(find_container(c), "Required component container not found");
    }
}

}
}

