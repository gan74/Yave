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

Archetype::Archetype(usize component_count, memory::PolymorphicAllocatorBase* allocator) :
        _component_count(component_count),
        _component_infos(std::make_unique<ComponentRuntimeInfo[]>(component_count)),
		_allocator(allocator) {
}

Archetype::~Archetype() {
	if(_component_infos) {
		log_msg(fmt("destroying arch with % components % entities", _component_count, size()));
		y_debug_assert(_chunk_data.is_empty() == !_last_chunk_size);
		if(!_chunk_data.is_empty()) {
			for(usize i = 0; i != _component_count; ++i) {
				_component_infos[i].destroy_indexed(_chunk_data.last(), 0, _last_chunk_size);
			}
			for(usize c = 0; c + 1 < _chunk_data.size(); ++c) {
				for(usize i = 0; i != _component_count; ++i) {
					_component_infos[i].destroy_indexed(_chunk_data[c], 0, entities_per_chunk);
				}
			}
		}

		for(void* c : _chunk_data) {
			if(c) {
				_allocator.deallocate(c, _chunk_byte_size);
			}
		}
	}
}

usize Archetype::size() const {
	if(_chunk_data.is_empty()) {
		return 0;
	}
	return (_chunk_data.size() - 1) * entities_per_chunk + _last_chunk_size;
}

usize Archetype::component_count() const {
	return _component_count;
}

core::Span<ComponentRuntimeInfo> Archetype::component_infos() const {
	return core::Span<ComponentRuntimeInfo>(_component_infos.get(), _component_count);
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
		const usize start = size();
		for(usize i = 0; i != first; ++i) {
			entities[i].archetype = this;
			entities[i].archetype_index = start + i;
		}
	}

	void* chunk_data = _chunk_data.last();
	for(usize i = 0; i != _component_count; ++i) {
		_component_infos[i].create_indexed(chunk_data, _last_chunk_size, first);
	}
	_last_chunk_size += first;

	if(first != entities.size()) {
		add_entities(core::MutableSpan(entities.begin() + first, entities.size() - first));
	}
}

void Archetype::remove_entity(EntityData& data) {
	y_debug_assert(data.archetype == this);

	if(!_last_chunk_size) {
		if(_chunk_data.last()) {
			_allocator.deallocate(_chunk_data.last(), _chunk_byte_size);
		}
		_chunk_data.pop();
		_last_chunk_size = entities_per_chunk;
	}

	y_debug_assert(_last_chunk_size != 0);
	y_debug_assert(!_chunk_data.is_empty());
	const usize last_index = _last_chunk_size - 1;
	if(data.archetype_index + 1 != size()) {
		const usize chunk_index = data.archetype_index / entities_per_chunk;
		const usize item_index = data.archetype_index % entities_per_chunk;
		for(usize i = 0; i != _component_count; ++i) {
			_component_infos[i].move_indexed(_chunk_data[chunk_index], item_index, _chunk_data.last(), last_index, 1);
		}
	}

	for(usize i = 0; i != _component_count; ++i) {
		_component_infos[i].destroy_indexed(_chunk_data.last(), last_index, 1);
	}
	--_last_chunk_size;
	data.invalidate();
}

void Archetype::sort_component_infos() {
	const auto cmp = [](const ComponentRuntimeInfo& a, const ComponentRuntimeInfo& b) { return a.type_id < b.type_id; };
	sort(_component_infos.get(), _component_infos.get() + _component_count, cmp);

	y_debug_assert(_chunk_byte_size == 0);
	for(usize i = 0; i != _component_count; ++i) {
		_component_infos[i].chunk_offset = _chunk_byte_size;

		const usize size = _component_infos[i].component_size;
		_chunk_byte_size = memory::align_up_to(_chunk_byte_size, size);
		_chunk_byte_size += size * entities_per_chunk;

		if(i && _component_infos[i - 1].type_id == _component_infos[i].type_id) {
			y_fatal("Duplicated component type.");
		}
	}
}

void Archetype::transfer_to(Archetype* other, core::MutableSpan<EntityData> entities) {
	Y_TODO(We create empty entities just to move into them)
	const usize start = other->size();
	other->add_entities(entities, false);

	usize other_index = 0;
	for(usize i = 0; i != _component_count; ++i) {
		const ComponentRuntimeInfo& info = _component_infos[i];
		ComponentRuntimeInfo* other_info = nullptr;
		for(; other_index != other->_component_count; ++other_index) {
			if(other->_component_infos[other_index].type_id == info.type_id) {
				other_info = &other->_component_infos[other_index];
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

bool Archetype::matches_type_indexes(core::Span<u32> type_indexes) const {
	y_debug_assert(std::is_sorted(type_indexes.begin(), type_indexes.end()));
	if(type_indexes.size() != _component_count) {
		return false;
	}
	for(usize i = 0; i != type_indexes.size(); ++i) {
		if(type_indexes[i] != _component_infos[i].type_id) {
			return false;
		}
	}
	return true;
}

void Archetype::add_chunk_if_needed() {
	if(_last_chunk_size == entities_per_chunk || _chunk_data.is_empty()) {
		add_chunk();
	}
}

void Archetype::add_chunk() {
	y_debug_assert(_last_chunk_size == entities_per_chunk || _chunk_data.is_empty());
	_last_chunk_size = 0;
	_chunk_data.emplace_back(_chunk_byte_size ? _allocator.allocate(_chunk_byte_size) : nullptr);

#ifdef Y_DEBUG
	std::memset(_chunk_data.last(), 0xBA, _chunk_byte_size);
#endif
}

}
}
