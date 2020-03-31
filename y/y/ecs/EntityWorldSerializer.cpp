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

#include "EntityWorldSerializer.h"

namespace y {
namespace ecs {
namespace detail {
static thread_local EntityWorld* thread_world = nullptr;

void set_serializer_world(EntityWorld* world) {
	y_debug_assert(!thread_world);
	thread_world = world;
}

void reset_serializer_world() {
	y_debug_assert(thread_world);
	thread_world = nullptr;
}

EntityWorld* serializer_world() {
	y_debug_assert(thread_world);
	return thread_world;
}
}


EntityIDSerializer::~EntityIDSerializer() {
}

void EntityIDSerializer::insert(u32 index) {
	while(true) {
		const EntityID id = detail::serializer_world()->create_entity();
		y_debug_assert(!id.version());
		if(id.index() == index) {
			return;
		}
		y_debug_assert(id.index() < index);
	}
}



ComponentSerializerBase::~ComponentSerializerBase() {
}

}
}
