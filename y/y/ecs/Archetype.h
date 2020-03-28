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
#ifndef Y_ECS_ARCHETYPE_H
#define Y_ECS_ARCHETYPE_H

#include "ComponentIterator.h"

#include <y/core/Range.h>
#include <y/core/Vector.h>
#include <y/mem/allocators.h>

#include <memory>

#ifdef Y_DEBUG
#include <y/utils/name.h>
#endif

namespace y {
namespace ecs {

struct ComponentRuntimeInfo {
	usize chunk_offset = 0;
	usize component_size = 0;

	void (*create)(void* dst, usize count) = nullptr;
	void (*create_from)(void* dst, void* from) = nullptr;
	void (*destroy)(void* ptr, usize count) = nullptr;
	void (*move)(void* dst, void* src, usize count) = nullptr;

	u32 type_id = u32(-1);

#ifdef Y_DEBUG
	std::string_view type_name = "unknown";
#endif

	void* index_ptr(void* chunk, usize index) const {
		return static_cast<u8*>(chunk) + chunk_offset + index * component_size;
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


	template<typename T>
	static ComponentRuntimeInfo from_type(usize offset = 0) {
		return {
			offset,
			sizeof(T),
			[](void* dst, usize count) {
				y_debug_assert(usize(dst) % sizeof(T) == 0);
				T* it = static_cast<T*>(dst);
				const T* end = it + count;
				for(; it != end; ++it) {
					::new(it) T();
				}
			},
			[](void* dst, void* from) {
				y_debug_assert(usize(dst) % sizeof(T) == 0);
				::new(dst) T(std::move(*static_cast<T*>(from)));
			},
			[](void* ptr, usize count) {
				y_debug_assert(usize(ptr) % sizeof(T) == 0);
				T* it = static_cast<T*>(ptr);
				const T* end = it + count;
				for(; it != end; ++it) {
					it->~T();
				}
#ifdef Y_DEBUG
				std::memset(ptr, 0xFE, count * sizeof(T));
#endif
			},
			[](void* dst, void* src, usize count) {
				y_debug_assert(usize(dst) % sizeof(T) == 0);
				T* it = static_cast<T*>(src);
				const T* end = it + count;
				T* out = static_cast<T*>(dst);
				while(it != end) {
					*out++ = std::move(*it++);
				}
			},
			type_index<T>(),

#ifdef Y_DEBUG
			ct_type_name<T>()
#endif
		};
	}
};

class Archetype : NonMovable {

	public:
		template<typename... Args>
		static std::unique_ptr<Archetype> create() {
			auto arc = std::make_unique<Archetype>(sizeof...(Args));
			arc->set_types<0, Args...>();
			return arc;
		}

		Archetype(Archetype&&) = default;
		Archetype& operator=(Archetype&&) = default;

		~Archetype();

		usize size() const;
		usize component_count() const;

		core::Span<ComponentRuntimeInfo> component_infos() const;

		void add_entity(EntityData& data);
		void add_entities(core::MutableSpan<EntityData> entities);


		template<typename... Args>
		auto view() {
			using begin_iterator = ComponentIterator<Args...>;
			using end_iterator = ComponentEndIterator;

			begin_iterator begin;
			if(!build_iterator<0>(begin)) {
				return core::Range(begin_iterator(), end_iterator());
			}
			return core::Range(begin, end_iterator(size()));
		}

	public:
		// This can not be private because of make_unique
		Archetype(usize component_count, memory::PolymorphicAllocatorBase* allocator = memory::global_allocator());

	private:
		friend class EntityWorld;


		void add_entities(core::MutableSpan<EntityData> entities, bool update_data);

		void sort_component_infos();
		void transfer_to(Archetype* other, core::MutableSpan<EntityData> entities);
		bool matches_type_indexes(core::Span<u32> type_indexes) const;
		void add_chunk_if_needed();
		void add_chunk();
		void remove_entity(EntityData& data);





		template<usize I, typename... Args>
		static void add_infos(ComponentRuntimeInfo* infos) {
			if constexpr(I < sizeof...(Args)) {
				using component_type = std::tuple_element_t<I, std::tuple<Args...>>;
				*infos = ComponentRuntimeInfo::from_type<component_type>();
				add_infos<I + 1, Args...>(infos + 1);
			}
		}

		template<usize I, typename... Args>
		void set_types() {
			add_infos<0, Args...>(_component_infos.get());
			sort_component_infos();
		}

		template<typename T>
		const ComponentRuntimeInfo* info_or_null() const {
			const u32 index = type_index<T>();
			Y_TODO(binary search?)
			for(usize i = 0; i != _component_count; ++i) {
				if(_component_infos[i].type_id == index) {
					return &_component_infos[i];
				}
			}
			return nullptr;
		}

		template<typename T>
		const ComponentRuntimeInfo* info() {
			if(const ComponentRuntimeInfo* i = info_or_null<T>()) {
				return i;
			}
			y_fatal("Unknown component type.");
		}

		template<usize I, typename... Args>
		[[nodiscard]] bool build_iterator(ComponentIterator<Args...>& it) {
			static_assert(sizeof...(Args));
			if constexpr(I < sizeof...(Args)) {
				using reference = typename ComponentIterator<Args...>::reference;
				using type = remove_cvref_t<std::tuple_element_t<I, reference>>;
				const ComponentRuntimeInfo* type_info =  info_or_null<type>();
				if(!type_info) {
					return false;
				}
				it._offsets[I] = type_info->chunk_offset;
				return build_iterator<I + 1>(it);
			} else {
				it._chunks = _chunk_data.begin();
			}
			return true;
		}

		template<typename... Args>
		std::unique_ptr<Archetype> archetype_with() {
			static_assert(sizeof...(Args));
			auto arc = std::make_unique<Archetype>(_component_count + sizeof...(Args));
			std::copy_n(_component_infos.get(), _component_count, arc->_component_infos.get());
			add_infos<0, Args...>(arc->_component_infos.get() + _component_count);
			arc->sort_component_infos();
			return arc;
		}

		// Use FixedArray instead?
		usize _component_count = 0;
		std::unique_ptr<ComponentRuntimeInfo[]> _component_infos;

		core::Vector<void*> _chunk_data;
		usize _last_chunk_size = 0;

		memory::PolymorphicAllocatorContainer _allocator;
		usize _chunk_byte_size = 0;
};



}
}

#endif // Y_ECS_ARCHETYPE_H
