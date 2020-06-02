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
#ifndef Y_ECS_COMPONENTRUNTIMEINFO_H
#define Y_ECS_COMPONENTRUNTIMEINFO_H

#include "ecs.h"

#include <y/utils/name.h>

#include <memory>
#include <cstring>

namespace y {
namespace ecs {
namespace detail {


template<typename T>
std::unique_ptr<ComponentInfoSerializerBase> create_info_serializer();

template<typename T>
ComponentSerializerWrapper create_component_serializer(Archetype*);

template<typename T>
std::unique_ptr<ComponentContainerBase> create_component_container(const void*);

using create_component_container_t = std::unique_ptr<ComponentContainerBase>(*)(const void*);

template<typename T>
create_component_container_t create_component_container_func() {
	if constexpr(std::is_copy_constructible_v<T>) {
		return create_component_container<T>;
	}
	return nullptr;
}


Y_TODO(y_debug_assert(usize(dst) % sizeof(T) == 0) doesnt make sense)

template<typename T>
void create_component(void* dst, usize count) {
	y_debug_assert(usize(dst) % sizeof(T) == 0);
	T* it = static_cast<T*>(dst);
	const T* end = it + count;
	for(; it != end; ++it) {
		::new(it) T();
	}
}

template<typename T>
void create_component_from(void* dst, void* from) {
	y_debug_assert(usize(dst) % sizeof(T) == 0);
	::new(dst) T(std::move(*static_cast<T*>(from)));
}

template<typename T>
void destroy_component(void* ptr, usize count) {
	y_debug_assert(usize(ptr) % sizeof(T) == 0);
	T* it = static_cast<T*>(ptr);
	const T* end = it + count;
	for(; it != end; ++it) {
		it->~T();
	}
#ifdef Y_DEBUG
	std::memset(ptr, 0xFE, count * sizeof(T));
#endif
}

template<typename T>
void move_component(void* dst, void* src, usize count) {
	y_debug_assert(usize(dst) % sizeof(T) == 0);
	T* it = static_cast<T*>(src);
	const T* end = it + count;
	T* out = static_cast<T*>(dst);
	while(it != end) {
		*out++ = std::move(*it++);
	}
}

}


struct ComponentRuntimeInfo {
	usize chunk_offset = 0;
	usize component_size = 0;

	void (*create)(void* dst, usize count) = nullptr;
	void (*create_from)(void* dst, void* from) = nullptr;
	void (*destroy)(void* ptr, usize count) = nullptr;
	void (*move)(void* dst, void* src, usize count) = nullptr;

	std::unique_ptr<ComponentInfoSerializerBase> (*create_info_serializer)() = nullptr;
	ComponentSerializerWrapper (*create_component_serializer)(Archetype*) = nullptr;
	std::unique_ptr<ComponentContainerBase> (*create_component_container)(const void*) = nullptr;

	u32 type_id = u32(-1);

	std::string_view type_name = "unknown";


	template<typename T>
	static ComponentRuntimeInfo from_type(usize offset = 0) {

		return {
			offset,
			sizeof(T),
			detail::create_component<T>,
			detail::create_component_from<T>,
			detail::destroy_component<T>,
			detail::move_component<T>,
			detail::create_info_serializer<T>,
			detail::create_component_serializer<T>,
			detail::create_component_container_func<T>(),
			type_index<T>(),
			ct_type_name<T>()
		};
	}



	void* index_ptr(void* chunk, usize index) const {
		return static_cast<u8*>(chunk) + chunk_offset + index * component_size;
	}

	const void* index_ptr(const void* chunk, usize index) const {
		return static_cast<const u8*>(chunk) + chunk_offset + index * component_size;
	}

	void create_indexed(void* chunk, usize index, usize count) const {
		create(index_ptr(chunk, index), count);
	}

	void create_from_indexed(void* chunk, usize index, void* from) const {
		create_from(index_ptr(chunk, index), from);
	}

	void destroy_indexed(void* chunk, usize index, usize count) const {
		destroy(index_ptr(chunk, index), count);
	}

	void move_indexed(void* dst, void* chunk, usize index, usize count) const {
		move(dst, index_ptr(chunk, index), count);
	}

	void move_indexed(void* dst_chunk, usize dst_index, void* src_chunk, usize src_index, usize count) const {
		move(index_ptr(dst_chunk, dst_index), index_ptr(src_chunk, src_index), count);
	}
};

}
}

#endif // Y_ECS_COMPONENTRUNTIMEINFO_H
