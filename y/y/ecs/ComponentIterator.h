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
#ifndef Y_ECS_COMPONENTITERATOR_H
#define Y_ECS_COMPONENTITERATOR_H

#include "ecs.h"

#include <tuple>
#include <type_traits>

namespace y {
namespace ecs {

class Archetype;

template<typename... Args>
class ComponentIterator;


struct ComponentEndIterator {
	public:
		ComponentEndIterator() = default;

	private:
		friend class Archetype;

		template<typename... Args>
		friend class ComponentIterator;

		ComponentEndIterator(usize size) : _index(size) {
		}

		usize _index = 0;
};

template<typename... Args>
class ComponentIterator {
	public:
		static constexpr usize component_count = sizeof...(Args);

		using difference_type = usize;
		using iterator_category = std::random_access_iterator_tag;

		using reference = std::tuple<Args&...>;
		using value_type = reference;
		using pointer = value_type*;

		reference operator*() const {
			return make_refence_tuple<0>();
		}


		bool operator==(const ComponentEndIterator& other) const {
			return _index == other._index;
		}

		bool operator!=(const ComponentEndIterator& other) const {
			return !operator==(other);
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


}
}

#endif // Y_ECS_COMPONENTITERATOR_H
