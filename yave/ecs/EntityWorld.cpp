/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

std::string_view EntityWorld::component_type_name(ComponentTypeIndex index) const {
	static constexpr std::string_view no_name = "unknown";
	const ComponentContainerBase* cont = container(index);
	return cont ? cont->component_type_name() : no_name;
}


const ComponentContainerBase* EntityWorld::container(ComponentTypeIndex type) const {
	if(const auto it = _component_containers.find(type); it != _component_containers.end()) {
		return it->second.get();
	}
	return nullptr;
}

ComponentContainerBase* EntityWorld::container(ComponentTypeIndex type) {
	auto& container = _component_containers[type];
	if(!container) {
		return nullptr;
	}
	return container.get();
}

void EntityWorld::add_required_components(EntityId id) {
	for(const ComponentTypeIndex& tpe : _required_components) {
		create_component(id, tpe).ignore();
	}
}

void EntityWorld::flush_reload(AssetLoader& loader) {
	for(const auto& p : _component_containers) {
		p.second->post_deserialize_poly(loader);
	}
}

}
}
