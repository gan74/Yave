/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
	_component_type_indexes.reserve(max_entity_component_types);
}

const Entity* EntityWorld::entity(EntityId id) const {
	return _entities.get(id);
}

Entity* EntityWorld::entity(EntityId id) {
	return _entities.get(id);
}

EntityId EntityWorld::create_entity() {
	return _entities.add();
}

void EntityWorld::remove_entity(EntityId id) {
	_deletions << id;
}

ComponentId EntityWorld::add_component(ComponentContainerBase* container, EntityId id) {
	Entity* ent = entity(id);
	y_debug_assert(ent);

	TypeIndex type = index_for_type(container->type());
	if(ent->has_component(type)) {
		return ent->component_id(type);
	}
	ComponentId comp = container->create_component(id);
	ent->add_component(type, comp);
	return comp;
}

void EntityWorld::remove_component(ComponentContainerBase* container, ComponentId id) {
	Entity* ent = entity(container->parent(id));
	y_debug_assert(ent);

	TypeIndex type = index_for_type(container->type());
	ent->remove_component(type);
}

TypeIndex EntityWorld::index_for_type(std::type_index type) {
	if(auto it = _component_type_indexes.find(type); it != _component_type_indexes.end()) {
		return it->second;
	}
	usize index = _component_containers.size();
	_component_type_indexes[type] = TypeIndex{index};
	_component_containers.emplace_back();
	return TypeIndex{index};
}

void EntityWorld::flush() {
	for(EntityId id : _deletions) {
		if(const Entity* ent = entity(id)) {
			for(usize i = 0; i != max_entity_component_types; ++i) {
				ComponentId id = ent->component_id(TypeIndex{i});
				if(id.is_valid()) {
					_component_containers[i]->remove_component(id);
				}
			}
		}
		_entities.remove(id);
	}

	for(auto& container : _component_containers) {
		container->flush();
	}
}

}
}
