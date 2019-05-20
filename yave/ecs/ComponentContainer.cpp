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
static RegisteredContainerType* registered_types_head = nullptr;
struct MagicNumber{};

usize registered_types_count() {
	usize count = 0;
	for(auto* i = registered_types_head; i; i = i->next) {
		++count;
	}
	return count;
}

void register_container_type(RegisteredContainerType* type, u64 type_id, std::type_index type_index, create_container_t create_container) {
	for(auto* i = registered_types_head; i; i = i->next) {
		y_debug_assert(i->type_id != type_id);
	}
	y_debug_assert(!type->next);

	type->type_id = type_id;
	type->type_index = type_index;
	type->create_container = create_container;
	type->next = registered_types_head;
	registered_types_head = type;
}

serde2::Result serialize_container(WritableAssetArchive& writer, ComponentContainerBase* container) {
	u64 type_id = container->serialization_type_id();
	if(!writer(u64(type_hash<MagicNumber>())) || !writer(u64(type_id))) {
		return core::Err();
	}
	return container->serialize(writer);
}

std::unique_ptr<ComponentContainerBase> deserialize_container(ReadableAssetArchive& reader) {
	u64 magic = 0;
	u64 type_id = 0;
	if(!reader(magic) || magic != type_hash<MagicNumber>() || !reader(type_id)) {
		return nullptr;
	}
	for(auto* i = registered_types_head; i; i = i->next) {
		if(i->type_id == type_id) {
			auto cont = i->create_container();
			if(cont->deserialize(reader)) {
				return cont;
			}
			return nullptr;
		}
	}
	return nullptr;
}

std::unique_ptr<ComponentContainerBase> create_container(std::type_index type_index) {
	for(auto* i = registered_types_head; i; i = i->next) {
		if(i->type_index == type_index) {
			return i->create_container();
		}
	}
	return nullptr;
}

}
}
}
