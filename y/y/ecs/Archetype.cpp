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

#include "Archetype.h"

#include <y/utils/sort.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace y {
namespace ecs {

Archetype::Archetype(ArchetypeRuntimeInfo info, memory::PolymorphicAllocatorBase* allocator) :
		_info(std::move(info)),
		_allocator(allocator) {

	y_debug_assert([this] {
		const auto le = [](const auto& a, const auto& b) { return a.type_id < b.type_id; };
		return std::is_sorted(_info.component_infos().begin(), _info.component_infos().end(), le);
	}());
}

Archetype::~Archetype() {
	const auto components = _info.component_infos();
	if(components.size()) {
		//log_msg(fmt("destroying arch with % components % entities", components.size(), entity_count()));
		/*for(const ComponentRuntimeInfo& info : components) {
			log_msg(info.type_name);
		}*/


		y_debug_assert(_chunk_data.is_empty() == !_last_chunk_size);
		if(!_chunk_data.is_empty()) {
			for(const ComponentRuntimeInfo& info : components) {
				info.destroy_indexed(_chunk_data.last(), 0, _last_chunk_size);
			}
			for(usize c = 0; c + 1 < _chunk_data.size(); ++c) {
				for(const ComponentRuntimeInfo& info : components) {
					info.destroy_indexed(_chunk_data[c], 0, entities_per_chunk);
				}
			}
		}

		for(void* c : _chunk_data) {
			if(c) {
				_allocator.deallocate(c, _info.chunk_byte_size());
			}
		}

		if(_chunk_cache) {
			_allocator.deallocate(_chunk_cache, _info.chunk_byte_size());
		}
	}
}

usize Archetype::entity_count() const {
	if(_chunk_data.is_empty()) {
		return 0;
	}
	return (_chunk_data.size() - 1) * entities_per_chunk + _last_chunk_size;
}

usize Archetype::component_count() const {
	return _info.component_count();
}

const ArchetypeRuntimeInfo& Archetype::runtime_info() const {
	return _info;
}

core::Span<ComponentRuntimeInfo> Archetype::component_infos() const {
	return _info.component_infos();
}

const ComponentRuntimeInfo* Archetype::component_info(u32 type_id) const {
	return _info.info_or_null(type_id);
}

void Archetype::add_entity(EntityData& data) {
	add_entities(core::MutableSpan<EntityData>(data));
}

void Archetype::add_entities(core::MutableSpan<EntityData> entities) {
	add_entities(entities, true);
}

void Archetype::add_entities(core::MutableSpan<EntityData> entities, bool update_data) {
	add_chunk_if_needed();

	const usize left_in_chunk = entities_per_chunk - _last_chunk_size;
	const usize first = std::min(left_in_chunk, entities.size());

	if(update_data) {
		const usize start = entity_count();
		for(usize i = 0; i != first; ++i) {
			entities[i].archetype = this;
			entities[i].archetype_index = start + i;
		}
	}

	void* chunk_data = _chunk_data.last();
	for(const ComponentRuntimeInfo& info : component_infos()) {
		info.create_indexed(chunk_data, _last_chunk_size, first);
	}
	_last_chunk_size += first;

	if(first != entities.size()) {
		add_entities(core::MutableSpan(entities.begin() + first, entities.size() - first), update_data);
	}
}

void Archetype::remove_entity(EntityData& data) {
	y_debug_assert(data.archetype == this);

	if(!_last_chunk_size) {
		pop_chunk();
	}

	y_debug_assert(_last_chunk_size != 0);
	y_debug_assert(!_chunk_data.is_empty());
	const usize last_index = _last_chunk_size - 1;
	if(data.archetype_index + 1 != entity_count()) {
		const usize chunk_index = data.archetype_index / entities_per_chunk;
		const usize item_index = data.archetype_index % entities_per_chunk;
		for(const ComponentRuntimeInfo& info : component_infos()) {
			info.move_indexed(_chunk_data[chunk_index], item_index, _chunk_data.last(), last_index, 1);
		}
	}

	for(const ComponentRuntimeInfo& info : component_infos()) {
		info.destroy_indexed(_chunk_data.last(), last_index, 1);
	}
	--_last_chunk_size;
	data.invalidate();
}

void* Archetype::raw_component(const EntityData& data, u32 type_id) {
	y_debug_assert(data.archetype == this);

	if(const ComponentRuntimeInfo* info = component_info(type_id)) {
		const usize chunk_index = data.archetype_index / entities_per_chunk;
		const usize item_index = data.archetype_index % entities_per_chunk;
		void* chunk = _chunk_data[chunk_index];
		return info->index_ptr(chunk, item_index);
	}
	return nullptr;
}

EntityPrefab Archetype::create_prefab(const EntityData& data) {
	y_debug_assert(data.archetype == this);

	const usize chunk_index = data.archetype_index / entities_per_chunk;
	const usize item_index = data.archetype_index % entities_per_chunk;
	const void* chunk = _chunk_data[chunk_index];

	EntityPrefab prefab;
	for(const ComponentRuntimeInfo& info : component_infos()) {
		const void* comp_data = info.index_ptr(chunk, item_index);
		auto comp = info.create_component_container(comp_data);
		if(comp) {
			prefab._components.emplace_back(std::move(comp));
		} else {
			log_msg(fmt("Component % is not copyable and was excluded from prefab", info.type_name), Log::Warning);
		}
	}
	return prefab;
}

void Archetype::transfer_to(Archetype* other, core::MutableSpan<EntityData> entities) {
	Y_TODO(We create empty entities just to move into them)
	const usize start = other->entity_count();
	other->add_entities(entities, false);

	usize other_index = 0;

	const auto other_infos = other->component_infos();
	for(const ComponentRuntimeInfo& info : component_infos()) {
		const ComponentRuntimeInfo* other_info = nullptr;
		for(; other_index != other_infos.size(); ++other_index) {
			if(other_infos[other_index].type_id == info.type_id) {
				other_info = &other_infos[other_index];
				break;
			}
		}

		if(other_info) {
			y_debug_assert(other_info->type_id == info.type_id);
			for(usize e = 0; e != entities.size(); ++e) {
				EntityData& data = entities[e];
				const usize src_chunk_index = data.archetype_index / entities_per_chunk;
				const usize src_item_index = data.archetype_index % entities_per_chunk;

				const usize dst_index = start + e;
				const usize dst_chunk_index = dst_index / entities_per_chunk;
				const usize dst_item_index = dst_index % entities_per_chunk;

				void* dst = other_info->index_ptr(other->_chunk_data[dst_chunk_index], dst_item_index);
				info.move_indexed(dst, _chunk_data[src_chunk_index], src_item_index, 1);
			}
		}
	}

	Y_TODO(optimize this with create_from)
	for(usize e = 0; e != entities.size(); ++e) {
		EntityData& data = entities[e];
		const EntityID id = data.id;
		remove_entity(data);

		data.id = id;
		data.archetype = other;
		data.archetype_index = start + e;
	}
}
void Archetype::add_chunk_if_needed() {
	if(_last_chunk_size == entities_per_chunk || _chunk_data.is_empty()) {
		add_chunk();
	}
}

void Archetype::add_chunk() {
	const usize chunk_byte_size = _info.chunk_byte_size();

	y_debug_assert(chunk_byte_size != 0);
	y_debug_assert(_last_chunk_size == entities_per_chunk || _chunk_data.is_empty());

	_last_chunk_size = 0;
	_chunk_data.emplace_back(chunk_byte_size ? _allocator.allocate(chunk_byte_size) : nullptr);

#ifdef Y_DEBUG
	std::memset(_chunk_data.last(), 0x4C, chunk_byte_size);
#endif
}

void Archetype::pop_chunk() {
	y_debug_assert(!_last_chunk_size);
	y_debug_assert(!_chunk_data.is_empty());
	y_debug_assert(_chunk_data.last());

	void* data = _chunk_data.pop();

	if(!_chunk_data.is_empty()) {
		_last_chunk_size = entities_per_chunk;
	}

	y_debug_assert(data);

	if(_chunk_cache) {
		_allocator.deallocate(data, _info.chunk_byte_size());
	} else {
		_chunk_cache = data;
	}
}

void Archetype::set_entity_count(usize count) {
	core::MutableSpan<EntityData> entities(nullptr, count);
	add_entities(entities, false);
	y_debug_assert(entity_count() == count);
}


core::Vector<ComponentSerializerWrapper> Archetype::create_component_serializer_wrappers() const {
	const auto components = _info.component_infos();
	auto serializers = core::vector_with_capacity<ComponentSerializerWrapper>(components.size());
	for(const ComponentRuntimeInfo& info : components) {
		serializers.emplace_back(info.create_component_serializer(const_cast<Archetype*>(this)));
	}
	return serializers;
}

ComponentSerializerList Archetype::create_serializer_list() const {
	return ComponentSerializerList(this);
}

}
}
