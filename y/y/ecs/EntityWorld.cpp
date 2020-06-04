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


namespace y {
namespace ecs {

EntityWorld::EntityWorld() {
}

usize EntityWorld::entity_count() const {
	return _entities.size();
}

bool EntityWorld::exists(EntityID id) const {
	return _entities.contains(id);
}

EntityID EntityWorld::create_entity() {
	return _entities.create();
}

EntityID EntityWorld::create_entity(const Archetype& archetype) {
	const EntityID id = create_entity();
	for(const auto& info : archetype.component_infos()) {
		ComponentContainerBase* cont = find_or_create_container(info);
		cont->add(id);
	}
	return id;
}

EntityID EntityWorld::create_entity(const EntityPrefab& prefab) {
	const EntityID id = create_entity();
	for(const auto& comp : prefab.components()) {
		ComponentContainerBase* cont = find_or_create_container(comp->runtime_info());
		comp->add_to(id, cont);
	}
	return id;
}

void EntityWorld::remove_entity(EntityID id) {
	check_exists(id);
	for(auto& cont : _containers.values()) {
		cont->remove(id);
	}
	_entities.recycle(id);
}

EntityID EntityWorld::id_from_index(u32 index) const {
	return _entities.id_from_index(index);
}

EntityPrefab EntityWorld::create_prefab(EntityID id) const {
	check_exists(id);
	EntityPrefab prefab;
	for(auto& cont : _containers.values()) {
		if(!cont->contains(id)) {
			continue;
		}
		auto box = cont->create_box(id);
		if(!box) {
			log_msg(fmt("% is not copyable and was excluded from prefab", cont->component_type_name()), Log::Warning);
		}
		prefab.add(std::move(box));
	}
	return prefab;
}

std::string_view EntityWorld::component_type_name(ComponentTypeIndex type_id) const {
	const ComponentContainerBase* cont = find_container(type_id);
	return cont ? cont->component_type_name() : "";
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
		cont = info.create_type_container(this);
	}
	return cont.get();
}

void EntityWorld::check_exists(EntityID id) const {
	y_always_assert(exists(id), "Entity doesn't exists");
}

}
}
