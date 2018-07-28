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

	auto& comp = ent->_components[container->type()];
	if(!comp.is_valid()) {
		comp = container->create_component();
	}
	return comp;
}

void EntityWorld::remove_component(ComponentContainerBase* container, ComponentId id) {
	Entity* ent = entity(container->parent(id));
	y_debug_assert(ent);

	usize removed = ent->_components.erase(container->type());
	unused(removed);
	y_debug_assert(removed);
}

void EntityWorld::flush() {
	for(EntityId id : _deletions) {
		if(const Entity* ent = entity(id)) {
			for(const auto& component : ent->components()) {
				auto& container = _component_containers[component.first];
				container->remove_component(component.second);
			}
		}
		_entities.remove(id);
	}

	for(auto& container : _component_containers) {
		container.second->flush();
	}
}

}
}
