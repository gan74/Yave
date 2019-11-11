/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

namespace yave {
namespace ecs {

EntityWorld::EntityWorld() {
}

EntityId EntityWorld::create_entity() {
	EntityId id = _entities.create();
	add_required_components(id);
	return id;
}

void EntityWorld::remove_entity(EntityId id) {
	if(id.is_valid()) {
		_deletions << id;
	}
}

EntityId EntityWorld::id_from_index(EntityIndex index) const {
	return _entities.id_from_index(index);
}

bool EntityWorld::exists(EntityId id) const {
	return _entities.contains(id);
}

const EntityIdPool& EntityWorld::entities() const {
	return _entities;
}

void EntityWorld::flush() {
	y_profile();
	if(!_deletions.is_empty()) {
		for(const auto& c : _component_containers) {
			c.second->remove(_deletions);
		}
		for(EntityId id : _deletions) {
			_entities.recycle(id);
		}
		_deletions.clear();
	}
}

void EntityWorld::add(const EntityWorld& other) {
	y_profile();
	std::unordered_map<EntityIndex, EntityId> id_map;
	for(EntityId id : other.entities()) {
		id_map[id.index()] = create_entity();
	}
	for(const auto& other_container : other._component_containers) {
		std::unique_ptr<ComponentContainerBase>& this_container = _component_containers[other_container.first];
		if(!this_container) {
			this_container = detail::create_container(other_container.second->type());
		}
		this_container->add(other_container.second.get(), id_map);
	}
}

std::string_view EntityWorld::type_name(ComponentTypeIndex index) const {
	static constexpr std::string_view no_name = "unknown";
	const ComponentContainerBase* cont = container(index);
	return cont ? cont->type_name() : no_name;
}


const ComponentContainerBase* EntityWorld::container(ComponentTypeIndex type) const {
	if(auto it = _component_containers.find(type); it != _component_containers.end()) {
		return it->second.get();
	}
	return nullptr;
}

ComponentContainerBase* EntityWorld::container(ComponentTypeIndex type) {
	auto& container = _component_containers[type];
	if(!container) {
		container = detail::create_container(type);
	}
	return container.get();
}

void EntityWorld::add_required_components(EntityId id) {
	for(const ComponentTypeIndex& tpe : _required_components) {
		create_component(id, tpe).ignore();
	}
}

void EntityWorld::post_deserialization() {
	for(const auto& p : _component_containers) {
		p.second->post_deserialize(*this);
	}
}

struct EntityWorldHeader {
	y_serde2(serde2::check(fs::magic_number, AssetType::World, u32(1)))
};

serde2::Result EntityWorld::serialize(WritableAssetArchive& writer) const {
	if(!writer(EntityWorldHeader())) {
		return core::Err();
	}

	if(!writer(u64(_entities.size()))) {
		return core::Err();
	}

	for(EntityId id : _entities) {
		if(!writer(u64(id.index()))) {
			return core::Err();
		}
	}

	if(!writer(u32(_component_containers.size()))) {
		return core::Err();
	}
	for(const auto& container : _component_containers) {
		if(!detail::serialize_container(writer, container.second.get())) {
			return core::Err();
		}
	}
	return core::Ok();
}

serde2::Result EntityWorld::deserialize(ReadableAssetArchive& reader) {
	// _required_components is not serialized so we don't clear it
	auto required = std::move(_required_components);
	*this = EntityWorld();
	_required_components = std::move(required);

	{
		EntityWorldHeader header;
		if(!reader(header)) {
			return core::Err();
		}
	}

	u64 entity_count = 0;
	if(!reader(entity_count)) {
		return core::Err();
	}

	for(u64 i = 0; i != entity_count; ++i) {
		u64 index = 0;
		if(!reader(index)) {
			return core::Err();
		}
		if(!_entities.create_with_index(EntityIndex(index))) {
			return core::Err();
		}
	}
	y_debug_assert(_entities.size() == entity_count);

	u32 container_count = 0;
	if(!reader(container_count)) {
		return core::Err();
	}
	for(u32 i = 0; i != container_count; ++i) {
		if(auto container = detail::deserialize_container(reader)) {
			_component_containers[container->type()] = std::move(container);
		} else {
			log_msg("Component type can not be deserialized.", Log::Warning);
		}
	}

	for(const auto& p : _component_containers) {
		p.second->post_deserialize(*this);
		// do some checking
		for(EntityIndex i : p.second->indexes()) {
			EntityId id = EntityId::from_unversioned_index(i);
			if(!_entities.contains(id)) {
				return core::Err();
			}
		}
	}

	if(!_required_components.is_empty()) {
		for(EntityId id : entities()) {
			add_required_components(id);
		}
	}

	return core::Ok();
}






}
}
