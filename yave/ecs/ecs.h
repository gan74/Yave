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
#ifndef YAVE_ECS_ECS_H
#define YAVE_ECS_ECS_H

#include "SlotMap.h"

#include <bitset>

namespace yave {
namespace ecs {

class Entity;
class EntityWorld;

static constexpr usize max_entity_component_types = 64;

using ComponentBitmask = std::bitset<max_entity_component_types>;

struct TypeIndex {
	usize index;
	bool operator==(const TypeIndex& other) const { return index == other.index; }
	bool operator!=(const TypeIndex& other) const { return index != other.index; }
};

struct EntityTag {};
using EntityId = SlotMapId<EntityTag>;

struct ComponentTag {};
using ComponentId = SlotMapId<ComponentTag>;




template<typename T>
class TypedComponentId : public ComponentId {
	public:
		TypedComponentId() = default;

		const ComponentId& to_generic_id() const {
			return static_cast<const ComponentId&>(*this);
		}

	private:
		friend class EntityWorld;

		TypedComponentId(ComponentId id) : ComponentId(id) {
		}
};

static_assert(!std::is_constructible_v<TypedComponentId<int>, ComponentId>);
static_assert(std::is_constructible_v<ComponentId, TypedComponentId<int>>);

}
}

#endif // YAVE_ECS_ECS_H
