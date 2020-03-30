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

#include <y/utils/iter.h>

namespace y {
namespace ecs {

template<typename... Args>
class EntityIterator {

	using inner_iterator = ComponentIterator<Args...>;

	public:
		static constexpr usize component_count = inner_iterator::component_count;

		using difference_type = void;
		using reference = typename inner_iterator::reference;
		using value_type = typename inner_iterator::value_type;
		using pointer = typename inner_iterator::pointer;

		using iterator_category = std::forward_iterator_tag;

		EntityIterator(core::Span<std::unique_ptr<Archetype>> archetypes) : _archetypes(archetypes) {
			if(!_archetypes.is_empty()) {
				_components = archetype()->template view<Args...>();
				if(_components.is_empty()) {
					advance_archetype();
				}
			}
		}

		void advance() {
			y_debug_assert(!_components.is_empty());
			_components = core::Range(_components.begin() + 1, _components.end());
			if(_components.is_empty()) {
				advance_archetype();
			}
		}

		bool at_end() const {
			return _archetype_index >= _archetypes.size();
		}

		reference operator*() const {
			y_debug_assert(!_components.is_empty());
			y_debug_assert(!at_end());
			return *_components.begin();
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
			return _archetypes == other._archetype && _archetype_index == other._archetype_index && _components.size() == other._components.size();
		}

		bool operator!=(const EntityIterator& other) const {
			return !operator==(other);
		}

	private:
		void advance_archetype() {
			do {
				++_archetype_index;
				if(at_end()) {
					break;
				}
				_components = archetype()->template view<Args...>();
			} while(_components.is_empty());
		}

		const Archetype* archetype() const {
			return _archetypes[_archetype_index].get();
		}

		Archetype* archetype() {
			return _archetypes[_archetype_index].get();
		}

		ComponentView<Args...> _components;

		usize _archetype_index = 0;
		core::Span<std::unique_ptr<Archetype>> _archetypes;
};

template<typename... Args>
using EntityViewRange = core::Range<EntityIterator<Args...>, EndIterator>;

template<typename... Args>
struct EntityView : EntityViewRange<Args...> {
	EntityView() : EntityView(EntityIterator<Args...>({})) {
	}

	EntityView(EntityIterator<Args...> beg) : EntityViewRange<Args...>(std::move(beg), EndIterator()) {
	}
};

}
}

#endif // Y_ECS_ENTITYVIEW_H
