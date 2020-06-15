/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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

#include <yave/assets/AssetLoadingContext.h>


namespace yave {
namespace ecs {

EntityWorld::EntityWorld() {
}

EntityWorld::~EntityWorld() {
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
		std::swap(_entities, other._entities);
		std::swap(_required_components, other._required_components);
	}
	for(const ComponentTypeIndex c : _required_components) {
		y_debug_assert(find_container(c));
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
		ComponentContainerBase* cont = find_or_create_container(info);
		cont->add(*this, id);
	}
	return id;
}

EntityId EntityWorld::create_entity(const EntityPrefab& prefab) {
	const EntityId id = create_entity();
	for(const auto& comp : prefab.components()) {
		comp->add_to(*this, id);
	}
	return id;
}

void EntityWorld::remove_entity(EntityId id) {
	check_exists(id);
	for(auto& cont : _containers.values()) {
		cont->remove(id);
	}
	_entities.recycle(id);
}

EntityId EntityWorld::id_from_index(u32 index) const {
	return _entities.id_from_index(index);
}

EntityPrefab EntityWorld::create_prefab(EntityId id) const {
	check_exists(id);
	EntityPrefab prefab;
	for(auto& cont : _containers.values()) {
		if(!cont->contains(id)) {
			continue;
		}
		auto box = cont->create_box(id);
		if(!box) {
			log_msg(fmt("% is not copyable and was excluded from prefab", cont->runtime_info().type_name), Log::Warning);
		}
		prefab.add(std::move(box));
	}
	return prefab;
}

core::Span<ComponentTypeIndex> EntityWorld::required_components() const {
	return _required_components;
}

std::string_view EntityWorld::component_type_name(ComponentTypeIndex type_id) const {
	const ComponentContainerBase* cont = find_container(type_id);
	return cont ? cont->runtime_info().type_name : "";
}

const ComponentContainerBase* EntityWorld::find_container(ComponentTypeIndex type_id) const {
	if(const auto it = _containers.find(type_id); it != _containers.end()) {
		return it->second.get();
	}
	return nullptr;
}

ComponentContainerBase* EntityWorld::find_container(ComponentTypeIndex type_id) {
	if(const auto it = _containers.find(type_id); it != _containers.end()) {
		return it->second.get();
	}
	return nullptr;
}

ComponentContainerBase* EntityWorld::find_or_create_container(const ComponentRuntimeInfo& info) {
	auto& cont = _containers[info.type_id];
	if(!cont) {
		cont = info.create_type_container();
	}
	y_debug_assert(cont);
	y_debug_assert(cont->type_id() == info.type_id);
	return cont.get();
}

void EntityWorld::check_exists(EntityId id) const {
	y_always_assert(exists(id), "Entity doesn't exists");
}


void EntityWorld::flush_reload(AssetLoader& loader) {
	y_profile();
	for(const auto& cont : _containers) {
		AssetLoadingContext loading_ctx(&loader);
		cont.second->post_deserialize_poly(loading_ctx);
	}
}


void EntityWorld::post_deserialize() {
	core::ExternalHashMap<ComponentTypeIndex, std::unique_ptr<ComponentContainerBase>> patched;
	for(auto& cont : _containers.values()) {
		if(cont) {
			patched[cont->type_id()] = std::move(cont);
		}
	}
	_containers = std::move(patched);
	for(const ComponentTypeIndex c : _required_components) {
		y_debug_assert(find_container(c));
	}
}

}
}
