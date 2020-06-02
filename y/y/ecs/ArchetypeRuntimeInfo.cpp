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

#include "ArchetypeRuntimeInfo.h"

#include <y/utils/sort.h>

namespace y {
namespace ecs {


ArchetypeRuntimeInfo::ArchetypeRuntimeInfo(usize component_count) :
		_component_infos(std::make_unique<ComponentRuntimeInfo[]>(component_count)),
		_component_count(component_count) {
}

ArchetypeRuntimeInfo::ArchetypeRuntimeInfo(const ArchetypeRuntimeInfo& other) : ArchetypeRuntimeInfo(other._component_count) {
	_chunk_byte_size = other._chunk_byte_size;
	std::copy_n(other._component_infos.get(), _component_count, _component_infos.get());
}

const ComponentRuntimeInfo* ArchetypeRuntimeInfo::info_or_null(u32 type_id) const {
	// The specs seems to imply that this is ok...
	const auto cmp = [](const ComponentRuntimeInfo& info, u32 index) {
		return info.type_id < index;
	};
	const ComponentRuntimeInfo* info = std::lower_bound(begin(), end(), type_id, cmp);
	if(info != end() && info->type_id == type_id) {
		return info;
	}
	return nullptr;
}

ArchetypeRuntimeInfo ArchetypeRuntimeInfo::create(core::Span<ComponentRuntimeInfo> infos) {
	ArchetypeRuntimeInfo info(infos.size());
	std::copy(infos.begin(), infos.end(), info._component_infos.get());
	info.sort_component_infos();
	return info;
}

usize ArchetypeRuntimeInfo::component_count() const {
	return _component_count;
}

usize ArchetypeRuntimeInfo::chunk_byte_size() const {
	return _chunk_byte_size;
}

core::Span<ComponentRuntimeInfo> ArchetypeRuntimeInfo::component_infos() const {
	return core::Span<ComponentRuntimeInfo>(_component_infos.get(), _component_count);
}

const ComponentRuntimeInfo* ArchetypeRuntimeInfo::begin() const {
	return _component_infos.get();
}

const ComponentRuntimeInfo* ArchetypeRuntimeInfo::end() const {
	return _component_infos.get() + _component_count;
}

bool ArchetypeRuntimeInfo::operator<(const ArchetypeRuntimeInfo& other) const {
	const auto less = [](const ComponentRuntimeInfo& a, const ComponentRuntimeInfo& b) { return a.type_id < b.type_id; };
	return std::lexicographical_compare(begin(), end(), other.begin(), other.end(), less);
}

bool ArchetypeRuntimeInfo::operator==(const ArchetypeRuntimeInfo& other) const {
	const auto eq = [](const ComponentRuntimeInfo& a, const ComponentRuntimeInfo& b) { return a.type_id == b.type_id; };
	return _component_count == other._component_count && std::equal(begin(), end(), other.begin(), eq);
}

bool ArchetypeRuntimeInfo::operator!=(const ArchetypeRuntimeInfo& other) const {
	return !operator==(other);
}



void ArchetypeRuntimeInfo::sort_component_infos() {
	const auto less = [](const ComponentRuntimeInfo& a, const ComponentRuntimeInfo& b) { return a.type_id < b.type_id; };
	sort(_component_infos.get(), _component_infos.get() + _component_count, less);

	y_debug_assert(_chunk_byte_size == 0);
	for(usize i = 0; i != _component_count; ++i) {
		const usize size = _component_infos[i].component_size;
		_chunk_byte_size = memory::align_up_to(_chunk_byte_size, size);
		_component_infos[i].chunk_offset = _chunk_byte_size;
		_chunk_byte_size += size * entities_per_chunk;

		if(i && _component_infos[i - 1].type_id == _component_infos[i].type_id) {
			y_fatal("Duplicated component type: %.", _component_infos[i].type_name);
		}
	}
}

bool ArchetypeRuntimeInfo::matches_type_indexes(core::Span<u32> type_indexes) const {
	y_debug_assert(std::is_sorted(type_indexes.begin(), type_indexes.end()));
	if(type_indexes.size() != _component_count) {
		return false;
	}
	for(usize i = 0; i != _component_count; ++i) {
		if(type_indexes[i] != _component_infos[i].type_id) {
			return false;
		}
	}
	return true;
}

core::Vector<std::unique_ptr<ComponentInfoSerializerBase>> ArchetypeRuntimeInfo::create_serializers() const {
	auto serializers =  core::vector_with_capacity<std::unique_ptr<ComponentInfoSerializerBase>>(_component_count);
	for(usize i = 0; i != _component_count; ++i) {
		serializers.emplace_back(_component_infos[i].create_info_serializer());
	}
	return serializers;
}

 void ArchetypeRuntimeInfo::set_serializers(core::Vector<std::unique_ptr<ComponentInfoSerializerBase>> serializers) {
	 _component_count = serializers.size();
	 _component_infos = std::make_unique<ComponentRuntimeInfo[]>(_component_count);

	 for(usize i = 0; i != _component_count; ++i) {
		 _component_infos[i] = serializers[i]->create_runtime_info();
	 }
	 sort_component_infos();
 }


}
}
