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
#ifndef YAVE_ECS_ENTITYIDPOOL_H
#define YAVE_ECS_ENTITYIDPOOL_H

#include "EntityId.h"

#include <y/core/Vector.h>
#include <y/core/Result.h>

namespace yave {
namespace ecs {

class EntityIdPool {
	template<bool Const>
	class Iterator {
		public:
			using value_type = std::conditional_t<Const, const EntityId, EntityId>;
			using reference = value_type&;
			using pointer = value_type*;
			using difference_type = usize;
			using iterator_category = std::forward_iterator_tag;

			reference operator*() const {
				y_debug_assert(!at_end());
				y_debug_assert(_it->_index != EntityId::invalid_index);
				return *_it;
			}

			pointer operator->() const {
				y_debug_assert(!at_end());
				y_debug_assert(_it->_index != EntityId::invalid_index);
				return _it;
			}

			Iterator& operator++() {
				advance();
				return *this;
			}

			Iterator operator++(int) {
				Iterator it(*this);
				advance();
				return it;
			}

			bool operator==(const Iterator& other) const {
				return _it == other._it;
			}

			bool operator!=(const Iterator& other) const {
				return _it != other._it;
			}

			bool operator<(const Iterator& other) const {
				return _it < other._it;
			}

			bool operator>(const Iterator& other) const {
				return _it > other._it;
			}

			bool at_end() const {
				return _it->_version == EntityId::invalid_index;
			}

		private:
			friend class EntityIdPool;

			Iterator(pointer it) : _it(it) {
				skip_empty();
			}

			void advance() {
				++_it;
				skip_empty();
			}

			void skip_empty() {
				while(!at_end() && _it->_index == EntityId::invalid_index) {
					++_it;
				}
			}

			pointer _it;
	};

	public:
		using iterator = Iterator<false>;
		using const_iterator = Iterator<true>;

		EntityIdPool();

		core::Result<void> create_with_index(EntityIndex index);

		bool contains(EntityId id) const;

		EntityId create();
		void recycle(EntityId id);

		iterator begin();
		iterator end();
		const_iterator begin() const;
		const_iterator end() const;

		usize size() const;

	private:
		core::Vector<EntityId> _ids;
		core::Vector<EntityIndex> _free;

		usize _size = 0;
};

}
}

#endif // YAVE_ECS_ENTITYIDPOOL_H
