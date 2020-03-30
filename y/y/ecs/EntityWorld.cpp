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
	EntityData& ent = _entities.last();
	return ent.id = EntityID(_entities.size() - 1);
}

void EntityWorld::remove_entity(EntityID id) {
	check_exists(id);

	EntityData& data = _entities[id.index()];
	if(!data.archetype) {
		data.invalidate();
	} else {
		data.archetype->remove_entity(data);
	}
	y_debug_assert(!data.archetype);
	y_debug_assert(!data.is_valid());
}


core::Span<std::unique_ptr<Archetype>> EntityWorld::archetypes() const {
	return _archetypes;
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

void EntityWorld::check_exists(EntityID id) const {
	if(!exists(id)) {
		y_fatal("Entity doesn't exists.");
	}
}

}
}
