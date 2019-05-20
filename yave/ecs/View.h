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
#ifndef YAVE_ECS_VIEW_H
#define YAVE_ECS_VIEW_H

#include "ComponentContainer.h"

namespace yave {
namespace ecs {

template<bool Const, typename... Args>
class View {
	using vector_tuple = std::conditional_t<Const,
				std::tuple<const ComponentVector<Args>&...>,
				std::tuple<ComponentVector<Args>&...>>;

	using reference_tuple = std::conditional_t<Const,
				std::tuple<const Args&...>,
				std::tuple<Args&...>>;

	using index_type = ComponentVector<void>::index_type;
	using index_range = decltype(std::declval<ComponentVector<void>>().indexes());


	class EndIterator {};

	class Iterator {
		template<usize I = 0>
		auto make_refence_tuple(index_type index) const {
			if constexpr(I + 1 == sizeof...(Args)) {
				return std::make_tuple(std::get<I>(_vectors)[index]);
			} else {
				return std::tuple_cat(std::make_tuple(std::get<I>(_vectors)[*_it]),
									  make_refence_tuple<I + 1>(index));
			}
		}


		public:
			using value_type = reference_tuple;
			using reference = value_type;
			using difference_type = usize;
			using iterator_category = std::random_access_iterator_tag;

			reference operator*() const {
				return make_refence_tuple(*_it);
			}

			Iterator& operator++() {
				advance();
				return *this;
			}

			Iterator operator++(int) {
				Iterator it(*this);
				operator++();
				return it;
			}

			bool operator==(const Iterator& other) const {
				return _it == other._it;
			}

			bool operator!=(const Iterator& other) const {
				return _it != other._it;
			}

			bool operator==(const EndIterator&) const {
				return at_end();
			}

			bool operator!=(const EndIterator&) const {
				return !at_end();
			}

			bool at_end() const {
				return _it == _end;
			}

		private:
			friend class View;

			Iterator(index_range range, const vector_tuple& vecs) :
					_it(range.begin()),
					_end(range.end()),
					_vectors(vecs) {

				skip();
			}

			void advance() {
				++_it;
				skip();
			}

			void skip() {
				while(!at_end() && !matches()) {
					++_it;
				}
			}

			template<usize I = 0>
			bool matches() const {
				if constexpr(I != sizeof...(Args)) {
					return std::get<I>(_vectors).has(*_it) && matches<I + 1>();
				}
				return true;
			}

			typename index_range::const_iterator _it;
			typename index_range::const_iterator _end;
			vector_tuple _vectors;
	};


	template<usize I = 0>
	index_range shortest_range() {
		auto&& v = std::get<I>(_vectors);
		if constexpr(I + 1 == sizeof...(Args)) {
			return v.indexes();
		} else {
			index_range s = shortest_range<I + 1>();
			return s.size() < v.size() ? s : v.indexes();
		}
	}

	public:
		using iterator = Iterator;
		using const_iterator = Iterator;

		View(const vector_tuple& vecs) : _vectors(vecs), _short(shortest_range()) {
		}


		Iterator begin() const {
			return const_iterator(_short, _vectors);
		}

		EndIterator end() const {
			return EndIterator();
		}

	private:
		vector_tuple _vectors;
		index_range _short;
};

}
}

#endif // YAVE_ECS_VIEW_H
