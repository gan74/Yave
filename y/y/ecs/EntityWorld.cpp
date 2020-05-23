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

namespace y {
namespace ecs {

bool EntityWorld::exists(EntityID id) const {
	if(id.index() >= _entities.size()) {
		return false;
	}
	return _entities[id.index()].id.version() == id.version();
}

EntityID EntityWorld::create_entity() {
	_entities.emplace_back();
	EntityData& data = _entities.last();
	return data.id = EntityID(_entities.size() - 1);
}

EntityID EntityWorld::create_entity(const ArchetypeRuntimeInfo& archetype) {
	const EntityID id = create_entity();
	transfer(_entities[id.index()], find_or_create_archetype(archetype));

	for(const ComponentRuntimeInfo& info : archetype.component_infos()) {
		on_create(info.type_id, id);
	}

	return id;
}

void EntityWorld::remove_entity(EntityID id) {
	check_exists(id);

	EntityData& data = _entities[id.index()];
	if(!data.archetype) {
		data.invalidate();
	} else {
		for(const ComponentRuntimeInfo& info : data.archetype->component_infos()) {
			on_remove(info.type_id, id);
		}
		data.archetype->remove_entity(data);
	}

	y_debug_assert(!data.archetype);
	y_debug_assert(!data.is_valid());
}

const Archetype* EntityWorld::archetype(EntityID id) const {
	check_exists(id);
	return _entities[id.index()].archetype;
}

core::Span<std::unique_ptr<Archetype>> EntityWorld::archetypes() const {
	return _archetypes;
}

Archetype* EntityWorld::find_or_create_archetype(const ArchetypeRuntimeInfo& info) {
	for(auto&& arc : _archetypes) {
		if(arc->runtime_info() == info) {
			return arc.get();
		}
	}
	return _archetypes.emplace_back(Archetype::create(ArchetypeRuntimeInfo(info))).get();
}

void EntityWorld::transfer(EntityData& data, Archetype* to) {
	y_debug_assert(exists(data.id));
	y_debug_assert(data.archetype != to);

	if(data.archetype) {
		data.archetype->transfer_to(to, data);
	} else {
		to->add_entity(data);
	}

	y_debug_assert(data.archetype == to);
	y_debug_assert(exists(data.id));
}


void  EntityWorld::add_on_create(u32 type_id, CallbackFunc func) {
	while(_component_callbacks.size() <= type_id) {
		_component_callbacks.emplace_back();
	}
	_component_callbacks[type_id].on_create.emplace_back(std::move(func));
}

void  EntityWorld::add_on_remove(u32 type_id, CallbackFunc func) {
	while(_component_callbacks.size() <= type_id) {
		_component_callbacks.emplace_back();
	}
	_component_callbacks[type_id].on_remove.emplace_back(std::move(func));
}

const EntityWorld::ComponentCallBacks* EntityWorld::component_callbacks(u32 type_id) const {
	return type_id < _component_callbacks.size() ? &_component_callbacks[type_id] : nullptr;
}

void EntityWorld::on_create(u32 type_id, EntityID id) const {
	if(const ComponentCallBacks* callbacks = component_callbacks(type_id)) {
		for(const auto& on_create : callbacks->on_create) {
			on_create(*this, id);
		}
	}
}

void EntityWorld::on_remove(u32 type_id, EntityID id) const {
	if(const ComponentCallBacks* callbacks = component_callbacks(type_id)) {
		for(const auto& on_remove : callbacks->on_remove) {
			on_remove(*this, id);
		}
	}
}

void EntityWorld::check_exists(EntityID id) const {
	if(!exists(id)) {
		y_fatal("Entity doesn't exists.");
	}
}

void EntityWorld::post_deserialize() const {
	if(_component_callbacks.is_empty()) {
		return;
	}
	Y_TODO(This seems very slow...)
	for(const EntityID& id : entity_ids()) {
		const EntityData& data = _entities[id.index()];
		if(data.archetype) {
			for(const ComponentRuntimeInfo& info : data.archetype->component_infos()) {
				on_create(info.type_id, id);
			}
		}
	}
}

}
}
