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

#include <y/core/SparseVector.h>
#include "EntityIdPool.h"

namespace yave {
namespace ecs {

/*template<bool Const, typename... Args>
class View {
	using vector_tuple = std::conditional_t<Const,
				std::tuple<const core::SparseVector<Args>&...>,
				std::tuple<core::SparseVector<Args>&...>>;

	using reference_tuple = std::conditional_t<Const,
				std::tuple<const Args&...>,
				std::tuple<Args&...>>;

	template<typename F, usize I = 0>
	void for_each(F&& f) {
		if constexpr(I != sizeof...(Args)) {
			using type = std::tuple_element_t<I, pointer_tuple>;
			f(reintepret_cast<type>(_vectors[I]));
			for_each<F, I + 1>(y_fwd(f));
		}
	}

	class Iterator {
		public:
			using value_type = reference_tuple;
			using reference = value_type;
			using difference_type = usize;
			using iterator_category = std::random_access_iterator_tag;

			reference operator*() const {
				return std::apply(make_refence_tuple, _vectors);
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

			bool at_end() const {
				return at_end_t();
			}

		private:
			friend class View;

			Iterator(const EntityId2* it, usize shortest, const vector_tuple& vecs) :
					_it(it),
					_shortest(shortest),
					_vectors(vecs) {

				skip_empty();
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
			bool at_end_t() const {
				if constexpr(I != sizeof...(Args)) {
					if(I == shortest && _it == std::get<I>(_vectors).end()) {
						return true;
					}
					return at_end<I + 1>();
				}
				return true; // ?
			}

			template<usize I = 0>
			bool matches() const {
				if constexpr(I != sizeof...(Args)) {
					std::get<I>(_vectors.has(*_it)) && matches<F, I + 1>(y_fwd(f));
				}
			}

			const u32* _it = nullptr;
			usize _shortest = 0;
			vector_tuple _vectors;
	};


	void build() {
		if constexpr(I != sizeof...(Args)) {
			auto&& v = std::get<I>(_vectors);
			if(v.size() < _size) {
				_shortest = I;
				_size = v.size();
			}
			build<I + 1>();
		}
	}

	public:
		View(const vector_tuple& vecs) : _vectors(vecs) {
			build();
		}


	private:
		vector_tuple _vectors;
		usize _size = usize(-1);
		usize _shortest = 0;
};*/

}
}

#endif // YAVE_ECS_VIEW_H
