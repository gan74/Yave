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

#include "Entity.h"

namespace yave {
namespace ecs {

const ComponentBitmask& Entity::components_bits() const {
	return _component_type_bits;
}

EntityId Entity::id() const {
	return _id;
}

usize Entity::component_index(ComponentTypeIndex type) const {
	auto t = _component_type_bits << (_component_type_bits.size() - type.index);
	return t.count();
}

bool Entity::has_component(ComponentTypeIndex type) const {
	return _component_type_bits[type.index];
}

ComponentId Entity::component_id(ComponentTypeIndex type) const {
	if(has_component(type)) {
		return _components[component_index(type)];
	}
	return ComponentId();
}

void Entity::add_component(ComponentTypeIndex type, ComponentId id) {
	y_debug_assert(!_component_type_bits[type.index]);
	usize index = component_index(type);
	_component_type_bits[type.index] = true;
	_components.push_back(id);
	std::rotate(_components.begin() + index, _components.end() - 1, _components.end());
	y_debug_assert(_components[index] == id);
}

void Entity::remove_component(ComponentTypeIndex type) {
	if(has_component(type)) {
		_component_type_bits[type.index] = false;
		usize index = component_index(type);
		y_debug_assert(index < _components.size());
		_components.erase(_components.begin() + index);
	}
}

}
}
