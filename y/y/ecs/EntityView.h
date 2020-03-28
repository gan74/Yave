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
#ifndef Y_ECS_ENTITYVIEW_H
#define Y_ECS_ENTITYVIEW_H

#include "Archetype.h"

namespace y {
namespace ecs {


class EntityEndIterator {};

template<typename... Args>
class EntityIterator {

	using inner_iterator = ComponentIterator<Args...>;
	using inner_end_iterator = ComponentEndIterator;

	public:
		static constexpr usize component_count = inner_iterator::component_count;

		using reference = typename inner_iterator::reference;

		using iterator_category = std::forward_iterator_tag;

		EntityIterator(core::Span<std::unique_ptr<Archetype>> archetypes) : _archetypes(archetypes) {
			if(!_archetypes.is_empty()) {
				_range = archetype()->template view<Args...>();
				if(_range.is_empty()) {
					advance_archetype();
				}
			}
		}

		void advance() {
			y_debug_assert(!_range.is_empty());
			_range = core::Range(_range.begin() + 1, _range.end());
			if(_range.is_empty()) {
				advance_archetype();
			}
		}

		bool at_end() const {
			return _archetype_index >= _archetypes.size();
		}

		reference operator*() const {
			y_debug_assert(!_range.is_empty());
			y_debug_assert(!at_end());
			return *_range.begin();
		}

		EntityIterator& operator++() {
			advance();
			return *this;
		}

		EntityIterator& operator--() {
			advance();
			return *this;
		}

		EntityIterator operator++(int) {
			const auto it = *this;
			++*this;
			return it;
		}

		EntityIterator operator--(int) {
			const auto it = *this;
			--*this;
			return it;
		}

		bool operator==(const EntityIterator& other) const {
			return _archetypes == other._archetype && _archetype_index == other._archetype_index && _range.size() == other._range.size();
		}

		bool operator!=(const EntityIterator& other) const {
			return !operator==(other);
		}

		bool operator==(const EntityEndIterator&) const {
			return at_end();
		}

		bool operator!=(const EntityEndIterator&) const {
			return !at_end();
		}


	private:
		void advance_archetype() {
			do {
				++_archetype_index;
				if(at_end()) {
					break;
				}
				_range = archetype()->template view<Args...>();
			} while(_range.is_empty());
		}

		const Archetype* archetype() const {
			return _archetypes[_archetype_index].get();
		}

		Archetype* archetype() {
			return _archetypes[_archetype_index].get();
		}

		core::Range<inner_iterator, inner_end_iterator> _range = core::Range(inner_iterator(), inner_end_iterator());

		usize _archetype_index = 0;
		core::Span<std::unique_ptr<Archetype>> _archetypes;
};

}
}

#endif // Y_ECS_ENTITYVIEW_H
