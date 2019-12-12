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

#include <y/mem/allocators.h>
#include <y/core/Range.h>
#include <y/math/Vec.h>
#include <y/utils/log.h>
#include <y/utils/perf.h>
#include <y/utils/name.h>

#include <typeindex>
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


class Archetype;

static constexpr usize entities_per_chunk = 1024;



template<typename... Args>
struct ComponentIterator {
	public:
		static constexpr usize component_count = sizeof...(Args);

		using difference_type = usize;
		using iterator_category = std::random_access_iterator_tag;

		using reference = std::tuple<Args&...>;

		reference operator*() const {
			return make_refence_tuple<0>();
		}



		bool operator==(const ComponentIterator& other) const {
			return _index == other._index && _chunks == other._chunks;
		}

		bool operator!=(const ComponentIterator& other) const {
			return !operator==(other);
		}

		ComponentIterator& operator++() {
			++_index;
			return *this;
		}

		ComponentIterator& operator--() {
			--_index;
			return *this;
		}

		ComponentIterator operator++(int) {
			const auto it = *this;
			++*this;
			return it;
		}

		ComponentIterator operator--(int) {
			const auto it = *this;
			--*this;
			return it;
		}


		difference_type operator-(const ComponentIterator& other) const {
			return _index - other._index;
		}

		ComponentIterator& operator+=(usize n) {
			_index += n;
			return *this;
		}

		ComponentIterator& operator-=(usize n) {
			_index -= n;
			return *this;
		}


		ComponentIterator operator+(usize n) const {
			auto it = *this;
			it += n;
			return it;
		}

		ComponentIterator operator-(usize n) const {
			auto it = *this;
			it -= n;
			return it;
		}


	private:
		friend class Archetype;

		template<usize I = 0>
		auto make_refence_tuple() const {
			const usize chunk_index = _index / entities_per_chunk;
			const usize item_index = _index % entities_per_chunk;
			using type = std::remove_reference_t<std::tuple_element_t<I, reference>>;

			void* offset_chunk = static_cast<u8*>(_chunks[chunk_index]) + _offsets[I];
			type* chunk = static_cast<type*>(offset_chunk);
			if constexpr(I + 1 == component_count) {
				return std::tie(chunk[item_index]);
			} else {
				return std::tuple_cat(std::tie(chunk[item_index]),
									  make_refence_tuple<I + 1>());
			}
		}

		usize _index = 0;
		void** _chunks = nullptr;
		std::array<usize, component_count> _offsets;
};


class Archetype : NonCopyable {

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

		template<typename... Args>
		auto view() {
			ComponentIterator<Args...> begin;
			build_iterator<0>(begin);
			const auto end = begin + size();
			return core::Range(begin, end);
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
				using component_type = std::tuple_element_t<I, std::tuple<Args...>>;
				_component_offsets[I] = _chunk_byte_size;
				_component_infos[I] = ComponentRuntimeInfo::from_type<component_type>();
				_component_types.emplace_back(typeid(remove_cvref_t<component_type>));

				_chunk_byte_size = memory::align_up_to(_chunk_byte_size, sizeof(component_type));
				_chunk_byte_size += sizeof(component_type) * entities_per_chunk;

				set_types<I + 1, Args...>();
			}
		}

		template<typename T>
		usize type_index() const {
			const std::type_index type = typeid(T);
			for(usize i = 0; i != _component_count; ++i) {
				if(_component_types[i] == type) {
					return i;
				}
			}
			return y_fatal("Unknown component type.");
		}


		template<usize I, typename... Args>
		void build_iterator(ComponentIterator<Args...>& it) {
			if constexpr(I < sizeof...(Args)) {
				using reference = typename ComponentIterator<Args...>::reference;
				using type = remove_cvref_t<std::tuple_element_t<I, reference>>;
				it._offsets[I] = _component_offsets[type_index<type>()];
				build_iterator<I + 1>(it);
			} else {
				it._chunks = _chunk_data.begin();
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

		core::Vector<std::type_index> _component_types;

		memory::PolymorphicAllocatorContainer _allocator;
		usize _chunk_byte_size = 0;
};




int main() {
	Archetype arc = Archetype::create<int, float>();

	y_debug_assert(arc.size() == 0);

	arc.add_entity();
	arc.add_entity();

	y_debug_assert(arc.size() == 2);

	int base = 0;
	for(auto [i] : arc.view<int>()) {
		i = ++base;
	}

	for(auto [i] : arc.view<const int>()) {
		log_msg(fmt("% %", ct_type_name<decltype(i)>(), i));
	}
	return 0;
}



