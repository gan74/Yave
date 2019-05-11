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
#ifndef YAVE_ECS_MULTICOMPONENTITERATOR_H
#define YAVE_ECS_MULTICOMPONENTITERATOR_H

#include "ecs.h"
#include "ComponentBitmask.h"

namespace yave {
namespace ecs {

class MultiComponentIteratorEndSentry {};

template<typename It, bool Const = false>
class MultiComponentIterator {

		using entity_container = std::conditional_t<Const, const SlotMap<Entity, EntityTag>, SlotMap<Entity, EntityTag>>;
		using entity_container_ref = std::add_lvalue_reference_t<entity_container>;

	public:
		using value_type = std::conditional_t<Const, const Entity, Entity>;
		using reference = std::add_lvalue_reference_t<value_type>;
		using pointer = std::add_pointer_t<value_type>;
		using difference_type = usize;
		using iterator_category = std::forward_iterator_tag;
		using end_iterator = MultiComponentIteratorEndSentry;

		static_assert(std::is_same_v<typename std::iterator_traits<It>::value_type, EntityId>);

		bool at_end() const {
			return _iterator == _end;
		}

		const MultiComponentIterator& operator++() {
			do {
				++_iterator;
			} while(!at_end() && !has_all_bits());
			return *this;
		}

		MultiComponentIterator operator++(int) {
			MultiComponentIterator it = *this;
			operator++();
			return it;
		}

		pointer get() const {
			y_debug_assert(_iterator->is_valid());
			return _entities.get(*_iterator);
		}

		pointer operator->() const {
			return get();
		}

		reference operator*() const {
			return *get();
		}

		bool operator==(const MultiComponentIterator& other) const {
			return _iterator == other._iterator;
		}

		bool operator!=(const MultiComponentIterator& other) const {
			return _iterator != other._iterator;
		}

		bool operator==(const end_iterator&) const {
			return at_end();
		}

		bool operator!=(const end_iterator&) const {
			return !at_end();
		}

	private:
		friend class EntityWorld;

		MultiComponentIterator(It it, It end, entity_container_ref entities, const ComponentBitmask& bits) :
				_iterator(it),
				_end(end),
				_component_type_bits(bits),
				_entities(entities) {

			while(!at_end() && !has_all_bits()) {
				++_iterator;
			}
		}

		bool has_all_bits() const {
			return _iterator->is_valid() && (get()->components_bits() & _component_type_bits) == _component_type_bits;
		}


		It _iterator;
		It _end;
		ComponentBitmask _component_type_bits;
		entity_container_ref _entities;
};

}
}

namespace std {
template<typename It, bool Const>
struct iterator_traits<yave::ecs::MultiComponentIterator<It, Const>> {
	using iterator_type = yave::ecs::MultiComponentIterator<It, Const>;
	using value_type = typename iterator_type::value_type;
	using reference = typename iterator_type::reference;
	using pointer = typename iterator_type::pointer;
	using difference_type = typename iterator_type::difference_type;
	using iterator_category = typename iterator_type::iterator_category;
};

template<>
struct iterator_traits<yave::ecs::MultiComponentIteratorEndSentry> :
			iterator_traits<yave::ecs::MultiComponentIterator<yave::ecs::EntityId*>> {
};

}

#endif // YAVE_ECS_MULTICOMPONENTITERATOR_H
