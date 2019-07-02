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
				std::tuple<const ComponentVector<Args>*...>,
				std::tuple<ComponentVector<Args>*...>>;

	using reference_tuple = std::conditional_t<Const,
				std::tuple<const Args&...>,
				std::tuple<Args&...>>;

	using index_type = ComponentVector<void>::index_type;
	using index_range = decltype(std::declval<ComponentVector<void>>().indexes());

	class EndIterator {};




	template<typename It>
	struct ReturnComponents {
		using value_type = reference_tuple;
		using reference = value_type;

		reference operator*() const {
			return static_cast<const It*>(this)->components();
		}
	};

	template<typename It>
	struct ReturnIndex {
		using value_type = index_type;
		using reference = index_type;

		reference operator*() const {
			return static_cast<const It*>(this)->index();
		}
	};

	template<typename It>
	class IndexComponents {
		public:
			auto index() const {
				return _it->index();
			}

			auto components() const {
				return _it->components();
			}

			IndexComponents(const It* it) : _it(it) {
			}

		private:
			const It* _it;
	};

	template<typename It>
	struct ReturnIndexComponents {
		using value_type = IndexComponents<It>;
		using reference = IndexComponents<It>;

		reference operator*() const {
			return IndexComponents<It>(static_cast<const It*>(this));
		}
	};

	template<template<typename> class ReturnPolicy>
	class Iterator : public ReturnPolicy<Iterator<ReturnPolicy>> {
		template<usize I = 0>
		auto make_refence_tuple(index_type index) const {
			y_debug_assert(std::get<I>(_vectors));
			auto& v = *std::get<I>(_vectors);
			if constexpr(I + 1 == sizeof...(Args)) {
				return std::tie(v[index]);
			} else {
				return std::tuple_cat(std::tie(v[*_it]),
									  make_refence_tuple<I + 1>(index));
			}
		}

		public:
			using difference_type = usize;
			using iterator_category = std::input_iterator_tag;

			reference_tuple components() const {
				return make_refence_tuple(*_it);
			}

			index_type index() const {
				return *_it;
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
					y_debug_assert(std::get<I>(_vectors));
					return std::get<I>(_vectors)->has(*_it) && matches<I + 1>();
				}
				return true;
			}

			typename index_range::const_iterator _it;
			typename index_range::const_iterator _end;
			vector_tuple _vectors;
	};


	template<usize I = 0>
	index_range shortest_range() {
		auto* v = std::get<I>(_vectors);
		if(!v) {
			return index_range();
		}

		if constexpr(I + 1 == sizeof...(Args)) {
			return v->indexes();
		} else {
			index_range s = shortest_range<I + 1>();
			return s.size() < v->size() ? s : v->indexes();
		}
	}

	public:
		using iterator = Iterator<ReturnIndexComponents>;
		using const_iterator = Iterator<ReturnIndexComponents>;

		using component_iterator = Iterator<ReturnComponents>;
		using const_component_iterator = Iterator<ReturnComponents>;

		using const_index_iterator = Iterator<ReturnIndex>;

		using end_iterator = EndIterator;

		View(const vector_tuple& vecs) : _vectors(vecs), _short(shortest_range()) {
		}


		const_iterator begin() const {
			return const_iterator(_short, _vectors);
		}

		end_iterator end() const {
			return end_iterator();
		}

		auto components() const {
			return core::Range(const_component_iterator(_short, _vectors), end_iterator());
		}

		auto indexes() const {
			return core::Range(const_index_iterator(_short, _vectors), end_iterator());
		}



	private:
		vector_tuple _vectors;
		index_range _short;
};

}
}

#endif // YAVE_ECS_VIEW_H
