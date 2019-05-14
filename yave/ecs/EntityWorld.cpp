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

#include "EntityWorld.h"
#include "View.h"

namespace yave {
namespace ecs {

[[maybe_unused]] static void test_view() {
	core::SparseVector<float> a;
	core::SparseVector<int> b;
	a.insert(100, 13.0f);
	a.insert(1000, 7.0f);
	b.insert(1000, 4);
	b.insert(500, 6);
	b.erase(500);

	y_debug_assert(a.has(100));
	y_debug_assert(a.has(1000));
	y_debug_assert(!a.has(101));
	y_debug_assert(b.has(1000));
	y_debug_assert(!b.has(500));
	y_debug_assert(b[1000] == 4);


	log_msg(fmt("a: %", a.indexes()));
	log_msg(fmt("b: %", b.indexes()));

	View<true, float, int> v(std::make_tuple(a, b));

	for(std::tuple<const float&, const int&> p : v) {
		log_msg(fmt("%", p));
		unused(p);
	}
}

EntityWorld::EntityWorld() {
	_component_type_indexes.reserve(max_entity_component_types);
	test_view();
}

const Entity* EntityWorld::entity(EntityId id) const {
	return _entities.get(id);
}

Entity* EntityWorld::entity(EntityId id) {
	return _entities.get(id);
}

EntityId EntityWorld::create_entity() {
	EntityId id = _entities.insert();
	entity(id)->_id = id;
	return id;
}

void EntityWorld::remove_entity(EntityId id) {
	if(id.is_valid()) {
		_deletions << id;
	}
}

void EntityWorld::remove_component(ComponentContainerBase* container, EntityId id) {
	Entity* ent = entity(id);
	y_debug_assert(ent);
	y_debug_assert(container);

	ComponentTypeIndex type = container->type();
	ComponentId comp_id = ent->component_id(type);
	container->remove_component(comp_id);
}

ComponentTypeIndex EntityWorld::add_index_for_type(std::type_index type) {
	if(auto it = _component_type_indexes.find(type); it != _component_type_indexes.end()) {
		return it->second;
	}
	ComponentTypeIndex index = ComponentTypeIndex{_component_containers.size()};
	_component_type_indexes[type] = index;
	_component_containers.emplace_back();
	return index;
}

core::Result<ComponentTypeIndex> EntityWorld::index_for_type(std::type_index type) const {
	if(auto it = _component_type_indexes.find(type); it != _component_type_indexes.end()) {
		return core::Ok(it->second);
	}
	return core::Err();
}

void EntityWorld::flush() {
	y_profile();
	for(EntityId id : _deletions) {
		if(const Entity* ent = entity(id)) {
			for(usize i = 0; i != max_entity_component_types; ++i) {
				ComponentId id = ent->component_id(ComponentTypeIndex{i});
				if(id.is_valid()) {
					y_debug_assert(_component_containers[i]);
					_component_containers[i]->remove_component(id);
				}
			}
		}
		_entities.erase(id);
	}

	for(auto& container : _component_containers) {
		if(container) {
			container->flush();
		}
	}
}


core::String EntityWorld::type_name(ComponentTypeIndex index) const {
	for(const auto& p : _component_type_indexes) {
		if(p.second == index) {
			return y::detail::demangle_type_name(p.first.name());
		}
	}
	return "";
}

void EntityWorld::serialize(io::WriterRef writer) const {
	writer->write_one(u64(_entities.size()));
	for(const Entity& e : _entities) {
		writer->write_one(u64(e.id().full_id()));
	}


	usize non_null = std::count_if(_component_containers.begin(), _component_containers.end(), [](const auto& p) { return !!p; });
	writer->write_one(u32(non_null));
	for(const auto& container : _component_containers) {
		if(container) {
			detail::serialize_container(writer, container.get());
		}
	}
}

void EntityWorld::deserialize(io::ReaderRef reader) {
	*this = EntityWorld();
	u64 entity_count = reader->read_one<u64>();
	for(u64 i = 0; i != entity_count; ++i) {
		EntityId id = EntityId::from_full_id(reader->read_one<u64>());
		_entities.insert_with_id(id).unwrap();
		_entities[id]._id = id;
	}

	u32 container_count = reader->read_one<u32>();
	for(u32 i = 0; i != container_count; ++i) {
		if(auto container = detail::deserialize_container(reader, *this)) {
			ComponentTypeIndex type = container->type();

			y_debug_assert(_component_containers.size() > type.index);
			y_debug_assert(!_component_containers[type.index]);

			_component_containers[type.index] = std::move(container);
		}
	}
}

}
}
