/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "ComponentContainer.h"
#include "EntityWorld.h"

namespace yave {
namespace ecs {

namespace detail {
static RegisteredType* registered_types_head = nullptr;

void register_type(RegisteredType* type, usize hash, create_container_t create_container) {
	for(auto* i = registered_types_head; i; i = i->_next) {
		y_debug_assert(i->_hash != hash);
	}
	y_debug_assert(!type->_next);

	type->_hash = hash;
	type->_create_container = create_container;
	type->_next = registered_types_head;
	registered_types_head = type;
}

usize registered_types_count() {
	usize count = 0;
	for(auto* i = registered_types_head; i; i = i->_next) {
		++count;
	}
	return count;
}
}


ComponentContainerBase::ComponentContainerBase(EntityWorld& world, TypeIndex type) : _world(world), _type(type) {
}

ComponentContainerBase::~ComponentContainerBase() {
}

void ComponentContainerBase::set_parent(ComponentId id, EntityId parent) {
	while(_parents.size() <= id.index()) {
		_parents.emplace_back();
	}
	_parents[id.index()] = parent;
}

void ComponentContainerBase::unset_parent(ComponentId id) {
	if(id.index() < _parents.size()) {
		_parents[id.index()] = EntityId();
	}
}

void ComponentContainerBase::remove_component(ComponentId id) {
	if(id.is_valid()) {
		_deletions << id;
	}
}

core::ArrayView<EntityId> ComponentContainerBase::parents() const {
	return _parents;
}

EntityId ComponentContainerBase::parent(ComponentId id) const {
	u32 index = id.index();
	return index < _parents.size() ? _parents[index] : EntityId();
}

void ComponentContainerBase::finish_flush() {
	for(ComponentId id : _deletions) {
		Entity* entity = _world.entity(parent(id));
		entity->remove_component(type());
		unset_parent(id);
	}
	_deletions.clear();
}

TypeIndex ComponentContainerBase::type() const {
	return _type;
}

const EntityWorld& ComponentContainerBase::world() const {
	return _world;
}

}
}
