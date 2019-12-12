/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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


#include <y/core/FixedArray.h>
#include <y/core/Vector.h>

#include <y/math/Vec.h>
#include <y/utils/log.h>
#include <y/utils/perf.h>
#include <y/mem/allocators.h>

#include <thread>

using namespace y;

struct ComponentRuntimeInfo {
	usize component_size = 0;
	void (*create)(void* dst, usize count) = nullptr;
	void (*destroy)(void* ptr, usize count) = nullptr;
	void (*move)(void* dst, void* src, usize count) = nullptr;

	template<typename T>
	static ComponentRuntimeInfo from_type() {
		return {
			sizeof(T),
			[](void* dst, usize count) {
				T* it = static_cast<T*>(dst);
				const T* end = it + count;
				for(; it != end; ++it) {
					::new(it) T();
				}
			},
			[](void* ptr, usize count) {
				T* it = static_cast<T*>(ptr);
				const T* end = it + count;
				for(; it != end; ++it) {
					it->~T();
				}
			},
			[](void* dst, void* src, usize count) {
				const T* beg = static_cast<T*>(src);
				const T* end = beg + count;
				std::move(beg, end, static_cast<T*>(dst));
			},
		};
	}
};




class Archetype : NonCopyable {
	static constexpr usize entities_per_chunk = 1024;

	public:
		template<typename... Args>
		static Archetype create() {
			Archetype arc(sizeof...(Args));
			arc.set_types<0, Args...>();
			arc.add_chunk();
			return arc;
		}

		Archetype(Archetype&&) = default;
		Archetype& operator=(Archetype&&) = default;

		~Archetype() {
			for(usize i = 0; i != _component_count; ++i) {
				_component_infos[i].destroy(_chunk_data.last(), _last_chunk_size);
			}
			for(usize c = 0; c + 1 < _chunk_data.size(); ++c) {
				for(usize i = 0; i != _component_count; ++i) {
					_component_infos[i].destroy(_chunk_data[c], entities_per_chunk);
				}
			}

			for(void* c : _chunk_data) {
				_allocator.deallocate(c, _chunk_byte_size);
			}
		}

		usize size() const {
			return (_chunk_data.size() - 1) * entities_per_chunk + _last_chunk_size;
		}

		void add_entity() {
			if(_last_chunk_size == entities_per_chunk) {
				add_chunk();
			}
			u8* chunk_data = static_cast<u8*>(_chunk_data.last());
			for(usize i = 0; i != _component_count; ++i) {
				const usize offset = _component_offsets[i];
				_component_infos[i].create(chunk_data + offset, 1);
			}
			++_last_chunk_size;
		}

	private:
		Archetype(usize component_count) :
				_component_count(component_count),
				_component_offsets(std::make_unique<usize[]>(component_count)),
				_component_infos(std::make_unique<ComponentRuntimeInfo[]>(component_count)),
				_allocator(memory::global_allocator()) {
		}

		template<usize I, typename... Args>
		void set_types() {
			if constexpr(I < sizeof...(Args)) {
				_component_offsets[I] = _chunk_byte_size;
				_component_infos[I] = ComponentRuntimeInfo::from_type<std::tuple_element_t<I, std::tuple<Args...>>>();

				_chunk_byte_size = memory::align_up_to(_chunk_byte_size, _component_infos[I].component_size);
				_chunk_byte_size += _component_infos[I].component_size * entities_per_chunk;

				set_types<I + 1, Args...>();
			}
		}

		void add_chunk() {
			y_debug_assert(_last_chunk_size == entities_per_chunk || _chunk_data.is_empty());
			_last_chunk_size = 0;
			_chunk_data.emplace_back(_allocator.allocate(_chunk_byte_size));
		}

		usize _component_count = 0;
		std::unique_ptr<usize[]> _component_offsets;
		std::unique_ptr<ComponentRuntimeInfo[]> _component_infos;

		core::Vector<void*> _chunk_data;
		usize _last_chunk_size = 0;

		memory::PolymorphicAllocatorContainer _allocator;

		usize _chunk_byte_size = 0;
};




int main() {
	Archetype arc = Archetype::create<int, float>();

	y_debug_assert(arc.size() == 0);

	arc.add_entity();
	arc.add_entity();

	log_msg(fmt("% ", arc.size()));

	y_debug_assert(arc.size() == 2);

	return 0;
}



